#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "mod_plugins.h"
#include "cpu_state.h"
static unsigned char main_ram[0x200000],mod_ram[24],source[4096],decoded[64];
static unsigned calls,invalidations,served;
static unsigned char *at(uint32_t p) {
    if(p>=0x9F000000u && p<0x9F000018u) return mod_ram+p-0x9F000000u;
    assert(p>=0x80000000u && p<0x80200000u);return main_ram+p-0x80000000u;
}
uint8_t *memory_get_ram_ptr(void) { return main_ram; }
uint32_t psx_mod_read_word(uint32_t p) { uint32_t v;memcpy(&v,at(p),4);return v; }
uint8_t psx_mod_read_byte(uint32_t p) { return *at(p); }
void psx_mod_write_word(uint32_t p,uint32_t v) { memcpy(at(p),&v,4); }
void psx_mod_write_byte(uint32_t p,uint8_t v) { *at(p)=v; }
void dirty_ram_mark_executable_range(uint32_t p,uint32_t n) { assert(p+n<=sizeof main_ram);++invalidations; }
void psx_mod_counter_add(const char *name,uint32_t n) { (void)name;served+=n; }
int psx_mod_register_activation_plugin(const char *id,PSXModActivationCallback cb) { (void)id;(void)cb;return 1; }
int psx_mod_register_vblank_plugin(const char *id,PSXModVBlankCallback cb) { (void)id;(void)cb;return 1; }
uint32_t psx_mod_alloc_guest_memory(uint32_t n,uint32_t a) { assert(n==24 && a==4);return 0x9F000000u; }
int medievil2_seamless_prepare(void) { return 1; }
const unsigned char *medievil2_seamless_source(uint32_t lba,uint32_t n) {
    return ((lba==53383 && n==4096) || (lba==52111 && n==2048))?source:NULL;
}
const unsigned char *medievil2_seamless_decoded(const unsigned char *p,uint32_t n,
    uint32_t *length,uint32_t *prefix,uint32_t *cursor,uint32_t *remaining,uint32_t *bits) {
    if(n!=32 || memcmp(p,source,32)) return NULL;
    *length=64;*prefix=8;*cursor=16;*remaining=11;*bits=0x12340000;return decoded;
}
static int next(CPUState *cpu,uint32_t pc) { (void)cpu;(void)pc;++calls;return 0; }
int (*g_psx_bios_hle_hook)(CPUState *,uint32_t)=next;
#include "../src/mods/medievil2_seamless.c"
static void words(uint32_t p,uint32_t a,uint32_t b) { w32(p,a);w32(p+4,b); }
int main(void) {
    CPUState cpu={0};cpu.gpr[28]=0x800E0000;cpu.gpr[4]=0;cpu.gpr[31]=0x800A0500;
    memset(source,0x37,sizeof source);memset(decoded,0xA9,sizeof decoded);
    activate();tick();assert(g_psx_bios_hle_hook==dispatch);
    words(0x800A0654,0x27BDFFE0,0x3C06DEAD);
    words(0x800A06F0,0x27BDFFE0,0x3C03800F);
    w32(0x800F4938,53382);w32(0x800F4940,0x800BF6A4);
    uint32_t entry=0x800BF6A4;
    w32(entry+0xC,1);w32(entry+0x10,0x801000EC);w32(entry+0x18,3000);
    assert(dispatch(&cpu,0xA0654)==1 && cpu.gpr[2]==1);
    assert(!memcmp(at(0x801000EC),source,4096) && r32(completion)==0x4D324344);
    // Save/restore the full guest completion metadata before polling.
    unsigned char snapshot[24];memcpy(snapshot,mod_ram,24);memset(mod_ram,0,24);memcpy(mod_ram,snapshot,24);
    cpu.gpr[31]=0x800A0518;assert(dispatch(&cpu,0xA06F0)==1 && cpu.gpr[2]==0);
    assert(!r32(completion) && !r32(cpu.gpr[28]+0x84C));
    assert(dispatch(&cpu,0xA06F0)==0 && calls==1); // consumed once
    cpu.gpr[31]=0x800A0500;cpu.gpr[4]=0xFFFFFFFF;
    assert(dispatch(&cpu,0xA0654)==0);cpu.gpr[4]=0;
    w32(entry+0x10,0x801FF800);assert(dispatch(&cpu,0xA0654)==0); // overflow
    w32(entry+0x10,0x80020000);assert(dispatch(&cpu,0xA0654)==0); // code
    words(0x800ADFB4,0x27BDFFD0,0x00803821);
    memcpy(at(0x80100000),source,32);cpu.gpr[4]=0x80100000;cpu.gpr[5]=0x80100008;
    cpu.gpr[6]=32;cpu.gpr[31]=0x800A07FC;
    assert(dispatch(&cpu,0xADFB4)==1 && !memcmp(at(0x80100008),decoded,64));
    assert(r32(cpu.gpr[28]+0x86C)==0x80100010 && r32(cpu.gpr[28]+0x870)==11);
    cpu.gpr[31]=0x800A0B68;assert(dispatch(&cpu,0xADFB4)==0); // embedded error-screen decoder
    words(0x800AA244,0x27BDFFD8,0xAFB10014);
    // LBA 52111 -> 11:36:61 (BCD), rounded descriptor length 2000.
    uint32_t desc=0x801F0000;psx_mod_write_byte(desc,0x11);psx_mod_write_byte(desc+1,0x36);
    psx_mod_write_byte(desc+2,0x61);w32(desc+4,2000);
    cpu.gpr[4]=desc;cpu.gpr[5]=0x80110000;cpu.gpr[31]=0x800707C8;
    assert(dispatch(&cpu,0xAA244)==1 && !memcmp(at(0x80110000),source,2048));
    psx_mod_write_byte(desc+2,0x7A);assert(dispatch(&cpu,0xAA244)==0); // malformed BCD
    w32(0x800AA244,0);assert(dispatch(&cpu,0xAA244)==0); // alternate executable
    assert(invalidations==3 && served>=3);return 0;
}
