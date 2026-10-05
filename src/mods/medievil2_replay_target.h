#ifndef MEDIEVIL2_REPLAY_TARGET_H
#define MEDIEVIL2_REPLAY_TARGET_H

#include "cpu_state.h"
#include "mod_plugins.h"

static int medievil2_replay_ram(uint32_t address, unsigned bytes) {
    return address >= 0x80010000u && address < 0x80200000u &&
           bytes <= 0x80200000u - address;
}

/* Swap submits the completed bank's ordering tables into the other draw
 * buffer while displaying the completed bank. Replays must draw into the
 * displayed/captured rectangle. Change only the sandbox's SDK descriptors;
 * Swap still builds and submits its normal viewport commands and OT links. */
static int medievil2_replay_target(CPUState* cpu, uint32_t display, unsigned bank) {
    if (bank > 1 || !medievil2_replay_ram(display, 0xE4) ||
        !medievil2_replay_ram(cpu->gpr[28], 0x864)) return 0;
    uint32_t target = display + 0x18 + 92 * bank;
    uint32_t source = display + 0x18 + 92 * (bank ^ 1);
    int dx = (int16_t)psx_mod_read_half(target) - (int16_t)psx_mod_read_half(source);
    int dy = (int16_t)psx_mod_read_half(target + 2) - (int16_t)psx_mod_read_half(source + 2);
    if (psx_mod_read_word(target + 4) != psx_mod_read_word(source + 4)) return 0;
    uint32_t sentinel = cpu->gpr[28] + 0x814;
    uint32_t viewport = psx_mod_read_word(sentinel);
    unsigned n = 0;
    while (viewport != sentinel) {
        if (++n > 64 || !medievil2_replay_ram(viewport, 0x88)) return 0;
        uint32_t rect = viewport + 0x18 + 8 * bank;
        uint32_t offset = viewport + 0x28 + 4 * bank;
        int x = (int16_t)psx_mod_read_half(rect) + dx;
        int y = (int16_t)psx_mod_read_half(rect + 2) + dy;
        int ox = (int16_t)psx_mod_read_half(offset) + dx;
        int oy = (int16_t)psx_mod_read_half(offset + 2) + dy;
        if (x < 0 || y < 0 || x + psx_mod_read_half(rect + 4) > 1024 ||
            y + psx_mod_read_half(rect + 6) > 512 ||
            ox < -1024 || ox > 1023 || oy < -1024 || oy > 1023) return 0;
        psx_mod_write_half(rect, (uint16_t)x);
        psx_mod_write_half(rect + 2, (uint16_t)y);
        psx_mod_write_half(offset, (uint16_t)ox);
        psx_mod_write_half(offset + 2, (uint16_t)oy);
        /* Ask Swap to regenerate DR_AREA/DR_OFFSET even for a static viewport. */
        uint32_t flags = viewport + 0x48 + 28 * bank;
        psx_mod_write_word(flags, psx_mod_read_word(flags) | 1);
        viewport = psx_mod_read_word(viewport);
    }
    for (unsigned i = 0; i < 92; i += 4)
        psx_mod_write_word(source + i, psx_mod_read_word(target + i));
    return 1;
}
#endif
