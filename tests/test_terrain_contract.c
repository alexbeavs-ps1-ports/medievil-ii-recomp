#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu_state.h"
#include "mod_plugins.h"

static unsigned char memory[0x800000];
static unsigned offset(uint32_t p, unsigned bytes) {
    if (p >= 0x1F800000u && p + bytes <= 0x1F800400u)
        return 0x7FFC00u + (p - 0x1F800000u);
    assert(p >= 0x80000000u && p < 0x80800000u);
    assert(bytes <= 0x80800000u - p);
    return p - 0x80000000u;
}
uint8_t psx_mod_read_byte(uint32_t p) { return memory[offset(p, 1)]; }
uint16_t psx_mod_read_half(uint32_t p) {
    uint16_t value; memcpy(&value, memory + offset(p, 2), 2); return value;
}
uint32_t psx_mod_read_word(uint32_t p) {
    uint32_t value; memcpy(&value, memory + offset(p, 4), 4); return value;
}
void psx_mod_write_byte(uint32_t p, uint8_t value) { memory[offset(p, 1)] = value; }
void psx_mod_write_half(uint32_t p, uint16_t value) { memcpy(memory + offset(p, 2), &value, 2); }
void psx_mod_write_word(uint32_t p, uint32_t value) { memcpy(memory + offset(p, 4), &value, 4); }
uint32_t psx_mod_display_width(void) { return 512; }
uint32_t psx_mod_display_height(void) { return 240; }
int32_t psx_mod_widescreen_x_margin(void) { return 128; }
void psx_mod_counter_add(const char* name, uint32_t value) { (void)name; (void)value; }
int psx_mod_option_value(const char* package, const char* feature,
                         const char* option, char* out, uint32_t capacity) {
    (void)package; (void)feature; (void)option; (void)out; (void)capacity; return 0;
}
int psx_mod_register_activation_plugin(const char* id, PSXModActivationCallback callback) {
    (void)id; (void)callback; return 1;
}
void gte_write_ctrl(CPUState* cpu, uint8_t reg, uint32_t value) { cpu->gte_ctrl[reg] = value; }
int psx_mod_register_function_entry_plugin(const char* id, uint32_t p,
                                          PSXModFunctionEntryCallback callback) {
    (void)id; (void)p; (void)callback; return 1;
}
int psx_mod_register_function_filter_plugin(const char* id, uint32_t p,
                                           PSXModFunctionFilterCallback callback) {
    (void)id; (void)p; (void)callback; return 1;
}
#include "../src/mods/medievil2_terrain.c"

static void reset_visibility_records(uint32_t indices) {
    for (unsigned p = 0; p < 2; ++p) {
        psx_mod_write_word(CAPTURES+12*p, 1);
        psx_mod_write_word(CAPTURES+12*p+8, indices+2*p);
    }
}

static void test_visibility(void) {
    /* Scaled terrain view: all intermediate matrices use the same linear
     * interpolation contract as the draw replay. */
    TerrainView views[2] = {{{1.6,0,0, 0,1,0, 0,0,1}, {0,0,0}},
                            {{1.6,0,0, 0,1,0, 0,0,1}, {0,0,0}}};
    const double planes[5][3] = {
        {300,0,416}, {-300,0,416}, {0,300,152}, {0,-300,152}, {0,0,1}
    };
    int16_t lo[3] = {-20,-20,-2000}, hi[3] = {20,20,-1000};
    assert(scaled_view(&views[0]));
    assert(outside_views(views, planes, lo, hi)); /* entirely behind */
    hi[2] = 1000;
    assert(!outside_views(views, planes, lo, hi)); /* crosses near plane */
    lo[0] = 3500; hi[0] = 4500; lo[2] = 1000; hi[2] = 2000;
    assert(outside_views(views, planes, lo, hi));
    views[0].t[0] = -5000; /* visible earlier in the interpolated camera sweep */
    assert(!outside_views(views, planes, lo, hi));
    views[0].t[0] = 0;
    lo[0] = -50; hi[0] = 50; lo[1] = -6000; hi[1] = -5000;
    assert(outside_views(views, planes, lo, hi));
    hi[1] = 500; /* tall wall enters the view although its top is far outside */
    assert(!outside_views(views, planes, lo, hi));
    views[0].r[0] = 1;
    assert(!scaled_view(&views[0])); /* quaternion path must fall back */

    CPUState cpu = {0};
    const uint32_t geometry = 0x80050000, poly = 0x80051000;
    const uint32_t vertex = 0x80052000, indices = 0x80053000;
    psx_mod_write_word(geometry+0x10, poly); psx_mod_write_word(geometry+0x14, vertex);
    psx_mod_write_half(indices, 0); psx_mod_write_half(indices+2, 1);
    for (unsigned p = 0; p < 2; ++p) {
        for (unsigned j = 0; j < 4; ++j) {
            psx_mod_write_half(poly+16*p+2*j, 4*p+j);
            psx_mod_write_half(vertex+8*(4*p+j), (j&1) ? 50 : -50);
            psx_mod_write_half(vertex+8*(4*p+j)+2, (j&2) ? 50 : -50);
            psx_mod_write_half(vertex+8*(4*p+j)+4, p ? 1000 : -1000);
        }
        psx_mod_write_word(poly+16*p+12, 1);
        psx_mod_write_word(CAPTURES+12*p, 1);
        psx_mod_write_word(CAPTURES+12*p+4, 0);
        psx_mod_write_word(CAPTURES+12*p+8, indices+2*p);
    }
    /* Quad vertex 4 participates even when its other three corners are offscreen. */
    psx_mod_write_half(vertex+24+4, 500);
    assert(cell_bounds(indices, 1, poly, vertex, lo, hi) && hi[2] == 500);
    psx_mod_write_half(vertex+24+4, -1000);
    assert(!cell_bounds(0x801FFFFF, 1, poly, vertex, lo, hi));
    psx_mod_write_word(META, 2); psx_mod_write_word(TERRAIN+0x90, 2);
    psx_mod_write_word(0x8007FA74, 0x3C058030);
    cpu.gpr[4] = geometry;
    cpu.gte_ctrl[0] = 6554; cpu.gte_ctrl[2] = 4096; cpu.gte_ctrl[4] = 4096;
    cpu.gte_ctrl[24] = 256u<<16; cpu.gte_ctrl[25] = 120u<<16; cpu.gte_ctrl[26] = 300;
    psx_mod_write_word(META+64, 0);
    prune_terrain(&cpu);
    assert(psx_mod_read_word(CAPTURES) == 1); /* first frame has no history */
    prune_terrain(&cpu);
    assert(psx_mod_read_word(CAPTURES) == 0 && psx_mod_read_word(CAPTURES+12) == 1);
    assert(psx_mod_read_word(CAPTURES+8) == VISIBLE_INDICES && psx_mod_read_word(META) == 2);
    assert(psx_mod_read_half(VISIBLE_INDICES) == 1 && psx_mod_read_half(indices) == 0);
    assert(psx_mod_read_word(poly+12) == 1); /* only capture counts are changed */
    reset_visibility_records(indices);
    cpu.gte_ctrl[26] = 400; /* projection change invalidates the history */
    prune_terrain(&cpu);
    assert(psx_mod_read_word(CAPTURES) == 1);
    /* Translation sweep includes a formerly visible cell behind the new camera. */
    cpu.gte_ctrl[7] = 3000;
    reset_visibility_records(indices);
    prune_terrain(&cpu);
    assert(psx_mod_read_word(CAPTURES) == 1);
    cpu.gte_ctrl[7] = 0;
    reset_visibility_records(indices);
    prune_terrain(&cpu);
    assert(psx_mod_read_word(CAPTURES) == 1);
    /* Repeated indices are emitted once in original first-occurrence order. */
    reset_visibility_records(indices);
    psx_mod_write_half(indices, 1);
    prune_terrain(&cpu);
    assert(psx_mod_read_word(CAPTURES) == 1 && psx_mod_read_word(CAPTURES+12) == 0);
    assert(psx_mod_read_half(VISIBLE_INDICES) == 1 && psx_mod_read_half(indices+2) == 1);
    reset_visibility_records(indices);
    psx_mod_write_word(CAPTURES, POLYGON_CAPACITY);
    prune_terrain(&cpu);
    assert(psx_mod_read_word(CAPTURES) == POLYGON_CAPACITY);
    assert(psx_mod_read_word(CAPTURES+8) == indices); /* arena overflow falls back */
    puts("terrain visibility: authored bounds, near-plane crossing, camera sweep and fallback passed");
}

int main(void) {
    CPUState cpu = {0};
    const uint32_t vp = 0x80013000, camera = 0x80012000, matrix = 0x80014000;
    const uint32_t map = 0x80016000, ids = 0x80018000, cells = 0x80020000;
    const uint32_t polygons = 0x80022000, heap = 0x80023000, entry = 0x80080850;
    cpu.gpr[28] = 0x80011000;
    cpu.gpr[4] = vp; cpu.gpr[5] = 100; cpu.gpr[6] = TERRAIN + 0x94;
    psx_mod_write_word(entry, 0x27BDFED0); psx_mod_write_word(entry+4, 0xAFBF012C);
    psx_mod_write_word(0x8007FA74, 0x3C058030);
    psx_mod_write_word(0x8007ED4C, 0x27BDFFE0); psx_mod_write_word(0x8007ED50, 0xAFB00010);
    psx_mod_write_word(0x8007ED0C, 0x27BDFFE8); psx_mod_write_word(0x8007ED10, 0xAFBF0010);
    psx_mod_write_word(cpu.gpr[28]+0x64C, camera);
    psx_mod_write_word(cpu.gpr[28]+0x584, vp);
    psx_mod_write_word(camera+0x10, map); psx_mod_write_word(vp+0x80, matrix);
    psx_mod_write_half(vp+0x68, 256); psx_mod_write_half(vp+0x6A, 120);
    psx_mod_write_word(map+0x0C, ids); psx_mod_write_word(map+0x10, cells);
    psx_mod_write_word(TERRAIN, heap); psx_mod_write_word(TERRAIN+0x88, heap);
    psx_mod_write_half(TERRAIN+8, 5);
    psx_mod_write_word(TERRAIN+0x38, 300u<<16); psx_mod_write_word(TERRAIN+0x3C, 1000u<<16);
    for (unsigned i=0; i<GRID*GRID; ++i) psx_mod_write_half(ids+2*i, 0xFFFF);
    psx_mod_write_half(ids+2*(32*GRID+32), 0);
    psx_mod_write_half(ids+2*(32*GRID+33), 0); /* one shared source cell */
    psx_mod_write_half(ids+2*(33*GRID+32), 1); /* near but over polygon budget */
    psx_mod_write_half(ids+2*(0*GRID+0), 2);   /* outside conservative disk */
    for (unsigned i=0; i<3; ++i) {
        psx_mod_write_half(cells+12*i, i ? 3 : POLYGON_CAPACITY - 1);
        psx_mod_write_half(cells+12*i+2, 29000); /* tall geometry must survive */
        psx_mod_write_half(cells+12*i+4, 31000);
        psx_mod_write_byte(cells+12*i+6, 0xA0);
        psx_mod_write_word(cells+12*i+8, polygons+16*i);
    }
    assert(capture(&cpu, entry) == 1 && cpu.gpr[2] == 1);
    assert(psx_mod_read_word(META+4) == 3 && psx_mod_read_word(META+8) == POLYGON_CAPACITY - 1);
    assert(psx_mod_read_word(CAPTURES) == POLYGON_CAPACITY - 1 && psx_mod_read_word(CAPTURES+4) == 0);
    assert(psx_mod_read_word(CAPTURES+8) == polygons);
    assert(psx_mod_read_byte(cells+6) == 0xA1);
    assert(psx_mod_read_byte(cells+18) == 0xA0);
    assert(psx_mod_read_word(MARKED+4) == 0);
    assert(cpu.gte_ctrl[24] == 256u<<16 && cpu.gte_ctrl[25] == 120u<<16);
    begin_capture(&cpu, 0x8007ED4C);
    assert(psx_mod_read_byte(cells+6) == 0xA0 && psx_mod_read_word(META) == 0);
    assert(capture(&cpu, entry) == 1);
    teardown(&cpu, 0x8007ED0C);
    assert(psx_mod_read_word(META) == 0 && psx_mod_read_byte(cells+6) == 0xA0);
    assert(psx_mod_read_word(TERRAIN) == heap && psx_mod_read_word(TERRAIN+0x88) == POLYGONS);
    assert(psx_mod_read_word(cpu.gpr[28]+0x46C) == PRIMITIVE0);
    assert(psx_mod_read_word(cpu.gpr[28]+0x474) == PRIMITIVE0 + PRIMITIVE_BYTES);
    /* Reconfiguration never rewinds packets already emitted this frame. */
    psx_mod_write_word(0x1F80006C, PRIMITIVE0 + 1024);
    configure_arenas(&cpu);
    assert(psx_mod_read_word(0x1F80006C) == PRIMITIVE0 + 1024);
    /* Conservative radial capture can include more than 8192 candidates even
     * when only a fraction survive the stock frustum/near-plane funnel. Do
     * not lose an entire visible cell at the old candidate budget. */
    begin_capture(&cpu, 0x8007ED4C);
    psx_mod_write_half(cells, 4100);
    psx_mod_write_half(cells+12, 4200);
    assert(capture(&cpu, entry) == 1 && cpu.gpr[2] == 2);
    assert(psx_mod_read_word(META+8) == 8300 && psx_mod_read_word(META+12) == 0);
    assert(psx_mod_read_byte(cells+6) == 0xA1 && psx_mod_read_byte(cells+18) == 0xA1);
    teardown(&cpu, 0x8007ED0C);
    assert(psx_mod_read_byte(cells+6) == 0xA0 && psx_mod_read_byte(cells+18) == 0xA0);
    /* Unsupported data routes the stock 100-cell capture to the same arena. */
    cpu.gpr[4] = 0x801FFFFC; cpu.gpr[5] = 4096; cpu.gpr[6] = 0;
    assert(capture(&cpu, entry) == 0 && cpu.gpr[5] == 100 && cpu.gpr[6] == CAPTURES);
    /* An instruction mismatch leaves the original register contract intact. */
    psx_mod_write_word(0x8007FA74, 0x24250094);
    cpu.gpr[5] = 17; cpu.gpr[6] = 0x80030000;
    assert(capture(&cpu, entry) == 0 && cpu.gpr[5] == 17 && cpu.gpr[6] == 0x80030000);
    /* Extended reach retains both owned fog allocations and fixed table count.
     * Repeated frames restore the authored values before scaling them again. */
    const uint32_t fog_buffer = 0x80030000;
    psx_mod_write_word(FOG, fog_buffer); psx_mod_write_word(FOG+4, fog_buffer);
    psx_mod_write_word(FOG+8, fog_buffer+384); psx_mod_write_half(FOG+12, 128);
    psx_mod_write_word(vp+0x70, (8192u<<16)|1);
    psx_mod_write_word(TERRAIN+0x3C, 1000u<<16);
    psx_mod_write_word(TERRAIN+0x50, 500u<<16);
    psx_mod_write_word(TERRAIN+0x54, 1000u<<16);
    psx_mod_write_word(TERRAIN+0x58, 0);
    psx_mod_write_word(TERRAIN+0x5C, 1000u<<16);
    extend_distance(vp);
    assert(psx_mod_read_half(vp+0x70) == 3 && psx_mod_read_half(vp+0x72) == 32768);
    assert(psx_mod_read_word(TERRAIN+0x3C) == 3000u<<16);
    assert(psx_mod_read_half(FOG+12) == 128 && psx_mod_read_word(FOG) == fog_buffer);
    assert(psx_mod_read_word(FOG+8) == fog_buffer+384);
    assert(psx_mod_read_half(fog_buffer+12) == 200);
    restore_distance();
    assert(psx_mod_read_word(TERRAIN+0x3C) == 1000u<<16);
    assert(psx_mod_read_word(vp+0x70) == ((8192u<<16)|1));
    extend_distance(vp);
    assert(psx_mod_read_word(TERRAIN+0x3C) == 3000u<<16);
    psx_mod_write_word(TERRAIN+0x3C, 1400u<<16); /* independent authored change */
    restore_distance();
    assert(psx_mod_read_word(TERRAIN+0x3C) == 1400u<<16);
    puts("terrain contract: conservative height, deduplication, budget, ownership, cleanup and fallback passed");
    test_visibility();
    return 0;
}
