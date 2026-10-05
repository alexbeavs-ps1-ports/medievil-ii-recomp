#include "mod_plugins.h"
#include "cpu_state.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SCUS-94564 MED2.EXE: the stock capture emits twelve-byte records,
 * then marks byte 6 of their source cells. Preserve those contracts while
 * selecting a conservative footprint independent of the ground plane.
 * Expanded arenas never replace the guest heap allocation at TERRAIN+0. */
enum {
    TERRAIN = 0x800F3B74u, CAPTURES = 0x80300000u,
    MARKED = 0x80310000u, META = 0x80315000u, POLYGONS = 0x80318000u,
    PRIMITIVE0 = 0x80400000u, PRIMITIVE1 = 0x80500000u,
    /* The title's conservative 3x capture exceeds 8192 candidate polygons.
     * Dropping whole cells there can omit visible corridor edges. The index
     * arena has room for 16384 pointers; emitted packets retain their own
     * bounded one-megabyte arenas and the engine's allocation checks. */
    PRIMITIVE_BYTES = 0x100000u, POLYGON_CAPACITY = 16384u,
    CAPACITY = 4096u, GRID = 64u, FOG = 0x800F426Cu
};
_Static_assert(POLYGONS + POLYGON_CAPACITY * 4u <= PRIMITIVE0,
               "Candidate polygon pointers must not overlap primitive packets");
static const unsigned distance_scale = 3;

typedef struct Candidate { uint32_t cell; uint64_t distance; } Candidate;
static Candidate candidates[GRID * GRID];

static int retail_ram(uint32_t p, uint32_t bytes) {
    return p >= 0x80010000u && p < 0x80200000u && bytes <= 0x80200000u - p;
}

static int nearer(const void* a, const void* b) {
    const Candidate *x = a, *y = b;
    if (x->distance != y->distance) return x->distance < y->distance ? -1 : 1;
    return x->cell < y->cell ? -1 : x->cell != y->cell;
}

static void clear_marked(void) {
    unsigned count = psx_mod_read_word(META);
    if (count > CAPACITY) count = CAPACITY;
    for (unsigned i = 0; i < count; ++i) {
        uint32_t cell = psx_mod_read_word(MARKED + 4 * i);
        if (retail_ram(cell, 12))
            psx_mod_write_byte(cell + 6, psx_mod_read_byte(cell + 6) & 0xFEu);
    }
    psx_mod_write_word(META, 0);
    psx_mod_write_word(MARKED, 0);
}

static void configure_arenas(CPUState* cpu) {
    uint32_t gp = cpu->gpr[28];
    /* GP+468 remains the original allocation the guest's teardown frees.
     * GP+46C/470 and +474/478 are the two render bases and their end limits.
     * A frame may already contain stock packets when capture first runs:
     * keep their OT links and move only subsequent allocation to our arena. */
    psx_mod_write_word(gp + 0x46C, PRIMITIVE0);
    psx_mod_write_word(gp + 0x470, PRIMITIVE1);
    psx_mod_write_word(gp + 0x474, PRIMITIVE0 + PRIMITIVE_BYTES);
    psx_mod_write_word(gp + 0x478, PRIMITIVE1 + PRIMITIVE_BYTES);
    unsigned index = psx_mod_read_word(gp + 0x860) & 1;
    uint32_t base = index ? PRIMITIVE1 : PRIMITIVE0;
    uint32_t current = psx_mod_read_word(0x1F80006C);
    if (current < base || current >= base + PRIMITIVE_BYTES)
        psx_mod_write_word(0x1F80006C, base);
    psx_mod_write_word(0x1F800070, base + PRIMITIVE_BYTES);
    psx_mod_write_word(TERRAIN + 0x88, POLYGONS);
    psx_mod_write_half(TERRAIN + 8, POLYGON_CAPACITY);
}

/* Restore only values still owned by this adapter before the guest advances
 * its authored transitions. Base and applied values live in guest RAM so a
 * restored state cannot accidentally compound the distance multiplier. */
static void restore_distance(void) {
    if (!psx_mod_read_word(META + 16)) return;
    uint32_t vp = psx_mod_read_word(META + 20);
    for (unsigned i = 0; i < 3; ++i) {
        unsigned field = i == 0 ? 0x3C : i == 1 ? 0x50 : 0x54;
        if (psx_mod_read_word(TERRAIN + field) == psx_mod_read_word(META + 40 + 4 * i))
            psx_mod_write_word(TERRAIN + field, psx_mod_read_word(META + 28 + 4 * i));
    }
    if (retail_ram(vp, 0x74) && psx_mod_read_word(vp + 0x70) == psx_mod_read_word(META + 52))
        psx_mod_write_word(vp + 0x70, psx_mod_read_word(META + 24));
    psx_mod_write_word(META + 16, 0);
}

static void extend_distance(uint32_t vp) {
    const unsigned delta = 2;
    unsigned reach = psx_mod_read_half(vp + 0x72), shift = psx_mod_read_half(vp + 0x70);
    unsigned count = psx_mod_read_half(FOG + 12);
    uint32_t depth = psx_mod_read_word(FOG + 4), brightness = psx_mod_read_word(FOG + 8);
    /* The retail terrain funnel copies two tables to fixed stack offsets.
     * Keep their original count and allocation; enlarge their depth steps
     * instead. The guarded disc patches use matching SRL 8 lookups. */
    if (reach != 8192 || count != 128 || shift + delta > 15 ||
        !retail_ram(depth, 2 * (count + 64)) ||
        brightness != depth + 2 * (count + 64) || !retail_ram(brightness, count + 64)) return;
    unsigned expanded = reach << delta;
    uint32_t fields[] = {0x3C, 0x50, 0x54};
    uint32_t scaled[3];
    for (unsigned i = 0; i < 3; ++i) {
        uint32_t original = psx_mod_read_word(TERRAIN + fields[i]);
        if ((int32_t)original < 0) return;
        uint64_t value = (uint64_t)original * distance_scale;
        uint32_t limit = (expanded - 1u) << 16;
        scaled[i] = value < limit ? (uint32_t)value : limit;
    }
    unsigned fog_shift = 22 + delta;
    unsigned start = scaled[1] >> fog_shift, finish = scaled[2] >> fog_shift;
    unsigned span = (scaled[2] - scaled[1]) >> fog_shift;
    if (scaled[2] <= scaled[1] || !span) return;
    int32_t value = (int32_t)psx_mod_read_word(TERRAIN + 0x58);
    int64_t difference = (int64_t)(int32_t)psx_mod_read_word(TERRAIN + 0x5C) - value;
    int32_t slope = (int32_t)(difference / span);
    psx_mod_write_word(META + 20, vp);
    psx_mod_write_word(META + 24, psx_mod_read_word(vp + 0x70));
    for (unsigned i = 0; i < 3; ++i) {
        psx_mod_write_word(META + 28 + 4 * i, psx_mod_read_word(TERRAIN + fields[i]));
        psx_mod_write_word(META + 40 + 4 * i, scaled[i]);
        psx_mod_write_word(TERRAIN + fields[i], scaled[i]);
    }
    psx_mod_write_half(vp + 0x70, shift + delta);
    psx_mod_write_half(vp + 0x72, expanded);
    psx_mod_write_word(META + 52, psx_mod_read_word(vp + 0x70));
    psx_mod_write_word(TERRAIN + 0x8C, (scaled[0] >> (16 + shift + delta)) - 1);
    for (unsigned i = 0; i < count; ++i) {
        uint16_t coefficient = (uint16_t)((uint32_t)value >> 16);
        psx_mod_write_half(depth + i * 2, coefficient);
        unsigned shade = coefficient >> 8;
        psx_mod_write_byte(brightness + i, shade < 15 ? shade : 15);
        if (i >= start && i <= finish) {
            int64_t next = (int64_t)value + slope;
            value = next > 0x10000000 ? 0x10000000 : (int32_t)next;
        }
    }
    psx_mod_write_word(META + 16, 1);
}

static void render(CPUState* cpu, uint32_t address) {
    (void)cpu;
    if (psx_mod_read_word(address) != 0x3C021F80u || !psx_mod_read_word(META + 16)) return;
    /* Terrain uses a previously staged viewport shift in scratchpad. Keep it
     * consistent with the unchanged OT capacity and the extended reach. */
    uint32_t vp = psx_mod_read_word(META + 20);
    if (retail_ram(vp, 0x74)) psx_mod_write_half(0x1F800064, psx_mod_read_half(vp + 0x70));
}

static void begin_capture(CPUState* cpu, uint32_t address) {
    (void)cpu;
    if (psx_mod_read_word(address) == 0x27BDFFE0u &&
        psx_mod_read_word(address + 4) == 0xAFB00010u) {
        clear_marked();
        restore_distance();
    }
}

static void teardown(CPUState* cpu, uint32_t address) {
    (void)cpu;
    if (psx_mod_read_word(address) == 0x27BDFFE8u &&
        psx_mod_read_word(address + 4) == 0xAFBF0010u) {
        clear_marked();
        restore_distance();
    }
}

static int capture(CPUState* cpu, uint32_t address) {
    if (psx_mod_read_word(address) != 0x27BDFED0u ||
        psx_mod_read_word(address + 4) != 0xAFBF012Cu ||
        psx_mod_read_word(0x8007FA74u) != 0x3C058030u) return 0;
    /* A validation failure runs the original bounded capture into the arena
     * the patched renderer reads. Its stock cleanup list still fits 100. */
    cpu->gpr[6] = CAPTURES;
    if (cpu->gpr[5] > 100) cpu->gpr[5] = 100;
    uint32_t vp = cpu->gpr[4];
    uint32_t camera = psx_mod_read_word(cpu->gpr[28] + 0x64C);
    if (!retail_ram(vp, 0x84) || !retail_ram(camera, 0x1C)) return 0;
    uint32_t matrix = psx_mod_read_word(vp + 0x80);
    uint32_t map = psx_mod_read_word(camera + 0x10);
    if (!retail_ram(matrix, 0x20) || !retail_ram(map, 0x14)) return 0;
    uint32_t ids = psx_mod_read_word(map + 0x0C);
    uint32_t cells = psx_mod_read_word(map + 0x10);
    if (!retail_ram(ids, GRID * GRID * 2) || !retail_ram(cells, 12)) return 0;
    extend_distance(vp);
    configure_arenas(cpu);

    int32_t cam_x = (int32_t)psx_mod_read_word(matrix + 0x14);
    int32_t cam_z = (int32_t)psx_mod_read_word(matrix + 0x1C);
    unsigned far = psx_mod_read_word(TERRAIN + 0x3C) >> 16;
    unsigned near = psx_mod_read_word(TERRAIN + 0x38) >> 16;
    unsigned budget = psx_mod_read_half(TERRAIN + 8);
    if (!far || far > 32767 || !near || !budget) return 0;
    unsigned width = psx_mod_display_width();
    int margin = psx_mod_widescreen_x_margin();
    double half = 160.0 + (width && margin > 0 ? (double)margin * 320 / width : 0);
    double radius = far * sqrt(1 + (half * half + 192.0 * 192) / ((double)near * near)) + 1536;
    uint64_t radius2 = (uint64_t)(radius * radius);
    unsigned found = 0;
    for (unsigned z = 0; z < GRID; ++z) for (unsigned x = 0; x < GRID; ++x) {
        int id = (int16_t)psx_mod_read_half(ids + 2 * (z * GRID + x));
        if (id < 0) continue;
        uint32_t cell = cells + (unsigned)id * 12;
        if (!retail_ram(cell, 12)) continue;
        unsigned count = psx_mod_read_half(cell);
        if (!count || !retail_ram(psx_mod_read_word(cell + 8), count * 2)) continue;
        /* The grid's signed world origin is -32768 on both horizontal axes. */
        int64_t dx = (int)x * 1024 - 32768 + 512 - (int64_t)cam_x;
        int64_t dz = (int)z * 1024 - 32768 + 512 - (int64_t)cam_z;
        uint64_t d2 = (uint64_t)(dx * dx) + (uint64_t)(dz * dz);
        if (d2 <= radius2) candidates[found++] = (Candidate){cell, d2};
    }
    qsort(candidates, found, sizeof candidates[0], nearer);
    unsigned captured = 0, spent = 0, skipped = 0;
    for (unsigned i = 0; i < found && captured < CAPACITY; ++i) {
        uint32_t cell = candidates[i].cell;
        if (psx_mod_read_byte(cell + 6) & 1) continue;
        unsigned count = psx_mod_read_half(cell);
        if (count > budget - spent) { ++skipped; continue; }
        uint32_t entry = CAPTURES + captured * 12;
        psx_mod_write_word(entry, count);
        /* No ground-height clipping: the stock polygon funnel still owns
         * near/depth/winding/vertical checks. Record+4 disables the optional
         * height prune; record+8 retains the original polygon-index list. */
        psx_mod_write_word(entry + 4, 0);
        psx_mod_write_word(entry + 8, psx_mod_read_word(cell + 8));
        psx_mod_write_word(MARKED + captured * 4, cell);
        psx_mod_write_byte(cell + 6, psx_mod_read_byte(cell + 6) | 1);
        spent += count;
        ++captured;
    }
    psx_mod_write_word(MARKED + captured * 4, 0);
    psx_mod_write_word(META, captured);
    psx_mod_write_word(META + 4, found);
    psx_mod_write_word(META + 8, spent);
    psx_mod_write_word(META + 12, skipped);
    /* The stock marked list stays empty; our entry/teardown hooks clear the
     * expanded one before the engine can release or reuse its source cells. */
    psx_mod_write_word(TERRAIN + 0x564, 0);
    uint32_t display = psx_mod_read_word(cpu->gpr[28] + 0x584);
    if (retail_ram(display, 0x6C)) {
        gte_write_ctrl(cpu, 24, (uint32_t)psx_mod_read_half(display + 0x68) << 16);
        gte_write_ctrl(cpu, 25, (uint32_t)psx_mod_read_half(display + 0x6A) << 16);
    }
    cpu->gpr[2] = captured;
    return 1;
}

PSX_MOD_CONSTRUCTOR(medievil2_register_terrain) {
    const char* plugin = "medievil2.widescreen";
    (void)psx_mod_register_function_entry_plugin(plugin, 0x8007ED4Cu, begin_capture);
    (void)psx_mod_register_function_entry_plugin(plugin, 0x8007ED0Cu, teardown);
    (void)psx_mod_register_function_filter_plugin(plugin, 0x80080850u, capture);
    (void)psx_mod_register_function_entry_plugin(plugin, 0x8007FA20u, render);
}
