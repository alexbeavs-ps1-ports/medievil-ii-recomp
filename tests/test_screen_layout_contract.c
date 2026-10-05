#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "mod_plugins.h"
#include "cpu_state.h"
static uint8_t ram[0x800000], scratch[1024];
static unsigned panels, radial, centred_text;
int g_psx_render_pass_active;
static float observed_scale;
static uint8_t *at(uint32_t a) {
    if (a>=0x1F800000u && a<0x1F800400u) return scratch+a-0x1F800000u;
    assert(a>=0x80000000u && a<0x80800000u);
    return ram+a-0x80000000u;
}
uint32_t psx_mod_read_word(uint32_t a) { uint32_t v; memcpy(&v,at(a),4); return v; }
uint16_t psx_mod_read_half(uint32_t a) { uint16_t v; memcpy(&v,at(a),2); return v; }
static void word(uint32_t a,uint32_t v) { memcpy(at(a),&v,4); }
static void half(uint32_t a,uint16_t v) { memcpy(at(a),&v,2); }
uint32_t psx_mod_display_width(void) { return 512; }
uint32_t psx_mod_display_height(void) { return 240; }
int32_t psx_mod_widescreen_view_x_margin(void) { return 85; }
void psx_mod_tag_screen_mask_quad(uint32_t p) { assert(p>=0x80400000u); ++panels; }
void psx_mod_tag_radial_screen_mask_quad(uint32_t p,float scale) {
    assert(p>=0x80400000u); ++radial; observed_scale=scale;
}
void psx_mod_anchor_hud_primitive(uint32_t p,int anchor) {
    assert(p==0x80400004u && anchor==0); ++centred_text;
}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t a,
    PSXModFunctionEntryCallback cb) { (void)id; (void)a; (void)cb; return 1; }
int psx_mod_register_instruction_plugin(const char *id,uint32_t a,uint32_t expected,
    PSXModFunctionEntryCallback cb) { (void)id; (void)a; (void)expected; (void)cb; return 1; }
#include "../src/mods/medievil2_screen_layout.c"
static void guard(uint32_t a,uint32_t x,uint32_t y) { word(a,x); word(a+4,y); }
static void f4(uint32_t p) { word(p,5u<<24); word(p+4,0x28000000u); }
int main(void) {
    const uint32_t p=0x80400000u;
    word(SCRATCH_CURSOR,p); word(SCRATCH_END,p+0x100000u);
    guard(0x8001A2E4u,0x3C02800Fu,0x8C46F384u);
    guard(0x800456F8u,0x27BDFFF0u,0xAFB10004u);
    guard(0x800A4C50u,0x3C030400u,0x3C02800Fu);
    word(0x800EF384u,0x80100000u);
    half(0x80100000u,192); half(0x80100002u,192);
    CPUState cpu={0}, before=cpu;
    prepare_bars(&cpu,0x8001A2E4u);
    assert(panels==0); /* no tag until the producer has completed its writes */
    f4(p); f4(p+24);
    submit(&cpu,0x800A4C50u);
    assert(panels==2 && !memcmp(&cpu,&before,sizeof cpu));
    submit(&cpu,0x800A4C50u); assert(panels==2);
    /* Swap replays drawing before the live OT is submitted. The sandbox
     * reuses RAM but allocates its bars elsewhere; a pass must not consume
     * or tag the live frame's packets while those bytes contain world data. */
    prepare_bars(&cpu,0x8001A2E4u);
    g_psx_render_pass_active=1;
    begin_draw(&cpu,0x80050044u);
    word(SCRATCH_CURSOR,p+0x1000);
    word(p+4,0x3C262626u); word(p+28,0x3C262626u);
    prepare_bars(&cpu,0x8001A2E4u);
    f4(p+0x1000); f4(p+0x1018);
    submit(&cpu,0x800A4C50u); assert(panels==4);
    /* Another interpolation phase starts with an empty pass list. */
    begin_draw(&cpu,0x80050044u);
    prepare_bars(&cpu,0x8001A2E4u);
    submit(&cpu,0x800A4C50u); assert(panels==6);
    g_psx_render_pass_active=0;
    word(SCRATCH_CURSOR,p); f4(p); f4(p+24);
    submit(&cpu,0x800A4C50u); assert(panels==8);
    /* An aborted pass and a changed submit function also leave live
     * allocations pending, and the next pass drops its aborted list. */
    prepare_bars(&cpu,0x8001A2E4u);
    g_psx_render_pass_active=1;
    begin_draw(&cpu,0x80050044u);
    prepare_bars(&cpu,0x8001A2E4u);
    word(0x800A4C50u,0);
    submit(&cpu,0x800A4C50u); assert(panels==8);
    prepare_bars(&cpu,0x8001A2E4u);
    begin_draw(&cpu,0x80050044u);
    guard(0x800A4C50u,0x3C030400u,0x3C02800Fu);
    submit(&cpu,0x800A4C50u); assert(panels==8);
    g_psx_render_pass_active=0;
    begin_draw(&cpu,0x80050044u);
    submit(&cpu,0x800A4C50u); assert(panels==10);
    panels=2;
    /* Fully open bars, short arena, and a changed producer are all inert. */
    half(0x80100000u,240); half(0x80100002u,240);
    prepare_bars(&cpu,0x8001A2E4u); submit(&cpu,0x800A4C50u); assert(panels==2);
    half(0x80100000u,192); word(SCRATCH_END,p+48);
    prepare_bars(&cpu,0x8001A2E4u); submit(&cpu,0x800A4C50u); assert(panels==2);
    word(SCRATCH_END,p+0x100000u); word(0x8001A2E4u,0);
    prepare_bars(&cpu,0x8001A2E4u); submit(&cpu,0x800A4C50u); assert(panels==2);
    prepare_iris(&cpu,0x800456F8u);
    for (unsigned i=0;i<32;++i) {
        word(p+i*36,8u<<24); word(p+i*36+4,0x3A000000u);
        f4(p+0x480u+i*24);
    }
    /* A recycled textured/world command must not inherit a pending mask. */
    word(p+4,0x3E000000u);
    submit(&cpu,0x800A4C50u);
    assert(radial==63 && observed_scale>1.18f && observed_scale<1.19f);
    assert(psx_mod_read_word(SCRATCH_CURSOR)==p);
    assert(!memcmp(&cpu,&before,sizeof cpu));
    guard(0x8009F7F0u,0x30A500FFu,0x2402000Au);
    cpu.gpr[28]=0x800EEAC8u; cpu.gpr[4]=0x80101234u; cpu.gpr[5]='A';
    word(cpu.gpr[28]+0x590u,cpu.gpr[4]);
    prepare_subtitle_glyph(&cpu,0x8009F7F0u);
    word(p+4,0xE1000200u); word(p+8,0x64808080u);
    submit(&cpu,0x800A4C50u); assert(centred_text==1);
    cpu.gpr[4]++; /* unrelated font/world text is not owned by this subtitle */
    prepare_subtitle_glyph(&cpu,0x8009F7F0u);
    submit(&cpu,0x800A4C50u); assert(centred_text==1);
    cpu.gpr[4]--;
    cpu.gpr[5]=10; /* a line break allocates no sprite */
    prepare_subtitle_glyph(&cpu,0x8009F7F0u);
    submit(&cpu,0x800A4C50u); assert(centred_text==1);
    cpu.gpr[5]=0xE9; /* extended font characters retain the same contract */
    prepare_subtitle_glyph(&cpu,0x8009F7F0u);
    submit(&cpu,0x800A4C50u); assert(centred_text==2);
    /* A prospective glyph allocation reused for another command is inert. */
    prepare_subtitle_glyph(&cpu,0x8009F7F0u);
    word(p+8,0x28000000u);
    submit(&cpu,0x800A4C50u); assert(centred_text==2);
    word(p+8,0x64808080u);
    word(SCRATCH_END,p+24);
    prepare_subtitle_glyph(&cpu,0x8009F7F0u);
    submit(&cpu,0x800A4C50u); assert(centred_text==2);
    /* A live subtitle has the same ownership rule as a bar. The pass may
     * overwrite its RAM address; only the restored live glyph is anchored. */
    word(SCRATCH_END,p+0x100000u);
    prepare_subtitle_glyph(&cpu,0x8009F7F0u);
    g_psx_render_pass_active=1;
    begin_draw(&cpu,0x80050044u);
    word(p+8,0x28000000u);
    submit(&cpu,0x800A4C50u); assert(centred_text==2);
    g_psx_render_pass_active=0;
    word(p+8,0x64808080u);
    submit(&cpu,0x800A4C50u); assert(centred_text==3);
    return 0;
}
