#include "render_pass_replay.hpp"

namespace {
// Start after object updates and stop before the drawing epilogue. Camera,
// model and terrain projections then run again at the interpolated phase.
constexpr uint32_t Start=0x80050044, Stop=0x80050114;
constexpr uint32_t Swap=0x800A101C, DrawSync=0x800A3240;
PSXDrawReplay replay;
uint32_t ticks=0, bank=0, display=0;
void tick() { if(!g_psx_render_pass_active) ++ticks; }
void begin(CPUState* cpu,uint32_t) {
    if(g_psx_render_pass_active) return;
    bank=psx_mod_read_word(cpu->gpr[28]+0x860);
    display=psx_mod_read_word(cpu->gpr[28]+0x45C);
    const uint32_t physical=display&0x1FFFFFFF;
    if(bank>1 || physical<0x10000 || physical>0x1FFF00 ||
       psx_mod_read_word(Start)!=0x0C026E80 || psx_mod_read_word(Swap)!=0x27BDFFC0) {
        replay.invalidate(); return;
    }
    replay.capture(cpu,Start,Stop);
}
int pass(CPUState* cpu,void*,uint32_t alpha) {
    if(!replay.restore(cpu,alpha) || !replay.draw(cpu)) return 0;
    PSXDrawReplay::call(cpu,Swap);
    PSXDrawReplay::call(cpu,DrawSync,0);
    psx_mod_counter_add("medievil2.fr.interpolated",replay.stats.replayed);
    return 1;
}
void submit(CPUState* cpu,uint32_t) {
    if(g_psx_render_pass_active || cpu->gpr[31]!=0x8004FC14) return;
    const bool ready=replay.prepare(ticks);
    psx_mod_counter_add("medievil2.fr.frames",1);
    psx_mod_counter_add("medievil2.fr.projections",replay.stats.captured);
    psx_mod_counter_add("medievil2.fr.matched",replay.stats.matched);
    psx_mod_counter_add("medievil2.fr.moving",replay.stats.changed);
    if(!ready) return;
    const uint32_t area=display+0x18+92*bank;
    PSXModRenderPassFrame frame{};
    frame.struct_size=sizeof frame; frame.period_vblanks=replay.ticks;
    frame.x=psx_mod_read_half(area); frame.y=psx_mod_read_half(area+2);
    frame.w=psx_mod_read_half(area+4); frame.h=psx_mod_read_half(area+6);
    psx_mod_counter_add("medievil2.fr.passes",psx_mod_render_pass_frame(cpu,&frame,pass,nullptr));
}
void activate(unsigned fps) {
    replay.invalidate(); ticks=0;
    PSXDrawReplay::rate(fps,PSX_MOD_RENDER_PASS_FLIP_PENDING);
}
#define RATE(name,value) void rate_##name() { activate(value); }
RATE(display,0) RATE(60,60) RATE(120,120) RATE(144,144) RATE(240,240) RATE(360,360)
}
PSX_MOD_CONSTRUCTOR(medievil2_register_frame_rate) {
#define REGISTER(name) psx_mod_register_activation_plugin("medievil2.framerate." #name,rate_##name)
    REGISTER(display); REGISTER(60); REGISTER(120); REGISTER(144); REGISTER(240); REGISTER(360);
    psx_mod_register_vblank_plugin("medievil2.framerate.tick",tick);
    psx_mod_register_instruction_plugin("medievil2.framerate.capture",Start,0x0C026E80,begin);
    psx_mod_register_function_entry_plugin("medievil2.framerate.submit",Swap,submit);
}
