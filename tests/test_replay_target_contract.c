#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu_state.h"
#include "mod_plugins.h"

static unsigned char ram[0x200000];
static unsigned offset(uint32_t address, unsigned bytes) {
    assert(address >= 0x80000000u && address < 0x80200000u);
    assert(bytes <= 0x80200000u-address);
    return address-0x80000000u;
}
uint16_t psx_mod_read_half(uint32_t a) { uint16_t v; memcpy(&v,ram+offset(a,2),2);return v; }
uint32_t psx_mod_read_word(uint32_t a) { uint32_t v; memcpy(&v,ram+offset(a,4),4);return v; }
void psx_mod_write_half(uint32_t a,uint16_t v) { memcpy(ram+offset(a,2),&v,2); }
void psx_mod_write_word(uint32_t a,uint32_t v) { memcpy(ram+offset(a,4),&v,4); }
#include "../src/mods/medievil2_replay_target.h"

int main(void) {
    CPUState cpu={0};
    cpu.gpr[28]=0x80020000;
    const uint32_t display=0x80030000, first=0x80031000, second=0x80032000;
    const uint32_t sentinel=cpu.gpr[28]+0x814;
    for(unsigned bank=0;bank<2;++bank) {
        memset(ram,0,sizeof ram);
        psx_mod_write_word(sentinel,first);
        psx_mod_write_word(first,second);psx_mod_write_word(second,sentinel);
        for(unsigned b=0;b<2;++b) {
            uint32_t env=display+0x18+92*b;
            psx_mod_write_half(env+2,b?0:256);
            psx_mod_write_half(env+4,512);psx_mod_write_half(env+6,240);
            psx_mod_write_word(env+20,0x12345678+b); /* distinguish descriptor payloads */
            for(unsigned i=0;i<2;++i) {
                uint32_t vp=i?second:first, rect=vp+0x18+8*b, off=vp+0x28+4*b;
                psx_mod_write_half(rect, i?56:0);psx_mod_write_half(rect+2,b?256:0);
                psx_mod_write_half(rect+4,i?400:512);psx_mod_write_half(rect+6,i?180:240);
                psx_mod_write_half(off,i?56:0);psx_mod_write_half(off+2,(b?256:0)-(i?20:0));
                psx_mod_write_word(vp+0x48+28*b,0xA0);
            }
        }
        assert(medievil2_replay_target(&cpu,display,bank));
        int target_y=bank?0:256;
        for(unsigned i=0;i<2;++i) {
            uint32_t vp=i?second:first,rect=vp+0x18+8*bank,off=vp+0x28+4*bank;
            assert(psx_mod_read_half(rect+2)==target_y);
            assert((int16_t)psx_mod_read_half(off+2)==target_y-(i?20:0));
            assert(psx_mod_read_half(rect)==(i?56:0));
            assert(psx_mod_read_half(rect+4)==(i?400:512));
            assert(psx_mod_read_word(vp+0x48+28*bank)==0xA1);
            assert(psx_mod_read_word(vp+0x48+28*(bank^1))==0xA0);
        }
        assert(!memcmp(ram+offset(display+0x18,92),ram+offset(display+0x18+92,92),92));
        assert(psx_mod_read_word(first)==second && psx_mod_read_word(second)==sentinel);
    }
    assert(!medievil2_replay_target(&cpu,display,2));
    assert(!medievil2_replay_target(&cpu,0x801FFFF0,0));
    psx_mod_write_half(display+0x18+92+4,320);
    assert(!medievil2_replay_target(&cpu,display,0));
    psx_mod_write_half(display+0x18+92+4,512);
    psx_mod_write_word(first,0x801FFFF0);
    assert(!medievil2_replay_target(&cpu,display,0));
    psx_mod_write_word(first,first);
    assert(!medievil2_replay_target(&cpu,display,0)); /* bounded corrupt cycle */
    puts("replay target: both banks, multiple viewports, signed offsets and fallback passed");
    return 0;
}
