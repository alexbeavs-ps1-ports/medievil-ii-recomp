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
static unsigned hud_tags;
static uint32_t tagged_packet[256];
static int tagged_edge[256];
int g_psx_render_pass_active;
void psx_mod_counter_add(const char *name,uint32_t delta) { (void)name; (void)delta; }
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
    if (p==0x80400004u && anchor==0) ++centred_text;
    else { assert(hud_tags<256); tagged_packet[hud_tags]=p;
           tagged_edge[hud_tags++]=anchor; }
}
int psx_mod_register_activation_plugin(const char *id,PSXModActivationCallback cb) {
    (void)id; (void)cb; return 1;
}
int psx_mod_register_savestate_plugin(const char *id,PSXModActivationCallback cb) {
    (void)id; (void)cb; return 1;
}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t a,
    PSXModFunctionEntryCallback cb) { (void)id; (void)a; (void)cb; return 1; }
int psx_mod_register_instruction_plugin(const char *id,uint32_t a,uint32_t expected,
    PSXModFunctionEntryCallback cb) { (void)id; (void)a; (void)expected; (void)cb; return 1; }
#include "../src/mods/medievil2_screen_layout.c"
static void guard(uint32_t a,uint32_t x,uint32_t y) { word(a,x); word(a+4,y); }
static void f4(uint32_t p) { word(p,5u<<24); word(p+4,0x28000000u); }
static void hud_sprite(uint32_t p) {
    word(p,0x00800000u); /* type zero, opaque */
    word(p+4,0x06000000u); word(p+8,0xE1000200u);
    word(p+12,0); word(p+16,0x64808080u);
    word(p+20,0x001E0018u); word(p+24,0x12345678u); word(p+28,0x00100020u);
}
static void test_hud_records(void) {
    const uint32_t p=0x80402000u;
    /* One completed record of each shape: SPRT, FT4, G4, E1/NOP/G4, GT3.
     * The compound DMA length covers all commands, not just the E1. */
    const unsigned payload[5]={28,40,36,44,40};
    const uint32_t opcode[5]={0x64808080u,0x2C808080u,0x38808080u,
                              0x3A808080u,0x34808080u};
    for (unsigned type=0;type<5;++type) {
        memset(at(p),0,48); word(p,0x00800000u|type);
        word(p+4,(payload[type]/4-1)<<24);
        unsigned offset=(type==0 || type==3) ? 16 : 8;
        if (offset==16) word(p+8,0xE1000220u);
        word(p+offset,opcode[type]);
        HudRange range={p,p+4+payload[type],-1,0};
        unsigned before=hud_tags;
        assert(hud_packets(&range,0)); assert(hud_tags==before);
        assert(hud_packets(&range,1)); assert(hud_tags==before+1);
        assert(tagged_packet[before]==p+offset-4 && tagged_edge[before]==-1);
        word(p+4,12u<<24); assert(!hud_packets(&range,0));
        word(p+4,(payload[type]/4-1)<<24);
        word(p+offset,0xA0000000u); assert(!hud_packets(&range,0));
        word(p+offset,opcode[type]); --range.end;
        assert(!hud_packets(&range,0));
    }
    hud_tags=0;
}
static void test_hud(void) {
    CPUState cpu={0}; cpu.gpr[28]=0x800EEAC8u;
    const uint32_t panel=0x80500000u, first=panel+4, p=0x80401000u;
    word(cpu.gpr[28]+0x79Cu,panel); word(panel,first);
    word(SCRATCH_END,0x80410000u);
    clear_hud();
    for (unsigned i=0;i<14;++i) {
        uint32_t node=first+i*28;
        word(node+4,hud_callbacks[i]); word(node+8,i);
        word(cpu.gpr[28]+0x7B0u,node); cpu.gpr[2]=hud_callbacks[i];
        word(SCRATCH_CURSOR,p+i*64);
        begin_hud_widget(&cpu,0x80078A14u);
        hud_sprite(p+i*64); hud_sprite(p+i*64+32); /* original and fade copy */
        word(SCRATCH_CURSOR,p+(i+1)*64);
        end_hud_widget(&cpu,0x80078A44u);
    }
    assert(hud_count==14);
    CPUState before=cpu; uint8_t packets[14*64]; memcpy(packets,at(p),sizeof packets);
    submit_hud(); assert(hud_tags==28);
    for (unsigned i=0;i<14;++i) {
        assert(tagged_packet[i*2]==p+i*64+12);
        assert(tagged_packet[i*2+1]==p+i*64+44);
        assert(tagged_edge[i*2]==hud_edges[i] && tagged_edge[i*2+1]==hud_edges[i]);
    }
    assert(!memcmp(&cpu,&before,sizeof cpu) && !memcmp(packets,at(p),sizeof packets));
    /* Replay begins after HUD construction. It retains ownership and neither
     * consumes the live list nor changes the authority's packet coordinates. */
    g_psx_render_pass_active=1; begin_frame(&cpu,0x8004FFB4u);
    begin_draw(&cpu,0x80050044u); submit_hud(); assert(hud_tags==56);
    g_psx_render_pass_active=0; submit_hud(); assert(hud_tags==84);
    /* Even a valid replacement command must not acquire stale HUD ownership. */
    word(p+16,0x64404040u); submit_hud(); assert(hud_tags==110);
    word(p+16,0x64808080u);
    /* A malformed/recycled command rejects the entire widget, including an
     * otherwise valid first sprite: there must be no half-anchored group. */
    word(p+32+16,0x28000000u); submit_hud(); assert(hud_tags==136);
    begin_frame(&cpu,0x8004FFB4u); assert(hud_count==0); submit_hud(); assert(hud_tags==136);
    word(cpu.gpr[28]+0x7B0u,first); cpu.gpr[2]=hud_callbacks[0];
    word(SCRATCH_CURSOR,p); hud_sprite(p);
    word(first+8,14); begin_hud_widget(&cpu,0x80078A14u); assert(!hud_node);
    word(first+8,0); word(first+4,0x8003F028u);
    begin_hud_widget(&cpu,0x80078A14u); assert(!hud_node);
    word(first+4,hud_callbacks[0]); cpu.gpr[2]=0x8003F028u;
    begin_hud_widget(&cpu,0x80078A14u); assert(!hud_node);
    cpu.gpr[2]=hud_callbacks[0];
    begin_hud_widget(&cpu,0x80078A14u); assert(hud_node);
    word(SCRATCH_CURSOR,p-4); end_hud_widget(&cpu,0x80078A44u); assert(!hud_count);
    word(SCRATCH_CURSOR,p); begin_hud_widget(&cpu,0x80078A14u);
    clear_hud(); word(SCRATCH_CURSOR,p+32); end_hud_widget(&cpu,0x80078A44u);
    assert(!hud_count); /* reset/restore cannot commit a stale allocation */
}
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
    test_hud_records(); test_hud();
    return 0;
}
