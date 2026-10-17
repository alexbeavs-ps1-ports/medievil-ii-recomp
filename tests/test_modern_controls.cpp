#include "../src/mods/medievil2_modern_controls.cpp"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#expr); std::exit(1); } } while (0)
static PSXModActivationCallback start, camera_start;
static PSXModPadTransform pad_callbacks;
static PsxPlayerBinds bindings;
static PSXModMousePolicy mouse_callbacks;
static uint32_t scene_state, scene_flags, player_pointer = 0x80100000;
static uint8_t player_paused;
static int persistent, transitions, analog, suppressed_button;
extern "C" {
int psx_mod_register_activation_plugin(const char* id, PSXModActivationCallback cb) {
    if(!std::strcmp(id,"medievil2.camera-controls"))camera_start=cb;else start=cb;return 1;
}
uint32_t psx_mod_read_word(uint32_t a) {
    if(a==0x8004FB9C)return 0x8F840584;
    if(a==0x800F0604)return scene_state;
    if(a==0x800EFF08)return scene_flags;
    if(a==0x800EEFD8)return player_pointer;
    return 0;
}
uint8_t psx_mod_read_byte(uint32_t) {return player_paused;}
int psx_mod_current_option_value(const char* name,char* out,uint32_t cap) {
    if(!std::strcmp(name,"sensitivity")){std::snprintf(out,cap,"normal");return 1;}
    return 0;
}
int psx_mod_option_value(const char*,const char*,const char*,char*,uint32_t) {return 0;}
int psx_keybinds_set_session_player(int player,const PsxPlayerBinds* p) {CHECK(player==1);bindings=*p;return 1;}
int psx_mod_set_controller_mode_override(uint32_t player,uint32_t mode) {CHECK(player==0);analog=mode;return 1;}
int psx_mod_set_pad_transform(uint32_t,const PSXModPadTransform* p) {pad_callbacks=*p;return 1;}
int psx_mod_set_local_mouse_policy(const PSXModMousePolicy* p) {mouse_callbacks=*p;return 1;}
int psx_mod_set_local_mouse_persistent_capture(int on) {persistent=on;return 1;}
int psx_mod_set_local_mouse_capture_across_transitions(int on) {transitions=on;return 1;}
int psx_keybinds_control_suppressed(int player,SDL_Scancode sc) {CHECK(player==1);return sc==PSXKB_MOUSE_SC(suppressed_button);}
}
int main() {
    CHECK(camera_start && start);camera_start();
    PSXModPadFrame frame{sizeof frame,0,0xFFFF,128,128,230,40,PSX_MOD_PAD_DIGITAL,PSX_MOD_PAD_HOST_GAMEPAD,0,0};
    PSXModPadOutput pad{};pad.struct_size=sizeof pad;
    CHECK(pad_callbacks.transform(&frame,&pad));
    CHECK(pad.type==PSX_MOD_PAD_DUALSHOCK && pad.rx==26 && pad.ry==216);
    scene_state=1;CHECK(pad_callbacks.transform(&frame,&pad));CHECK(pad.rx==230 && pad.ry==40);scene_state=0;
    frame.host_flags|=PSX_MOD_PAD_HOST_SOURCE;pad.rx=77;
    CHECK(!pad_callbacks.transform(&frame,&pad) && pad.rx==77); // no replay double inversion
    start();
    CHECK(persistent && transitions && analog==PSX_MOD_CONTROLLER_ANALOG);
    CHECK(bindings.circle==SDL_SCANCODE_SPACE && bindings.cross==SDL_SCANCODE_UNKNOWN);
    CHECK(bindings.ls_up==SDL_SCANCODE_UNKNOWN && bindings.up==SDL_SCANCODE_UNKNOWN && bindings.r1==SDL_SCANCODE_LCTRL);
    uint8_t keys[512]{};
    PSXModPadOutput shortcuts{}; shortcuts.buttons=0xFFFF;
    keys[SDL_SCANCODE_E]=1;keyboard_actions(keys,true,&shortcuts);
    CHECK(shortcuts.buttons==0x7BFF); // native weapon-switch chord
    keys[SDL_SCANCODE_E]=0;keys[SDL_SCANCODE_5]=1;shortcuts.buttons=0xFFFF;
    keyboard_actions(keys,true,&shortcuts);CHECK(shortcuts.buttons==0xEBFF); // native Dan-Hand chord
    keys[SDL_SCANCODE_5]=0;keys[SDL_SCANCODE_6]=1;shortcuts.buttons=0xFFFF;
    keyboard_actions(keys,true,&shortcuts);CHECK(shortcuts.buttons==0xFFFE); // item selection
    keys[SDL_SCANCODE_6]=0;keys[SDL_SCANCODE_F]=1;shortcuts.buttons=0xFFFF;
    keyboard_actions(keys,false,&shortcuts);CHECK(shortcuts.buttons==0xBFFF); // menu confirm too
    keys[SDL_SCANCODE_F]=0;keys[SDL_SCANCODE_LALT]=1;shortcuts.buttons=0xFFFF;
    keyboard_actions(keys,false,&shortcuts);CHECK(shortcuts.buttons==0xFFFF); // no look chord in menus
    keyboard_actions(keys,true,&shortcuts);CHECK(shortcuts.buttons==0xFCFF);
    keys[SDL_SCANCODE_LALT]=0;keys[SDL_SCANCODE_F4]=1;shortcuts.buttons=0xFFFF;
    keyboard_actions(keys,true,&shortcuts);CHECK(shortcuts.buttons==0xFFF9);
    CHECK(mouse_callbacks.eligible(0xFFFF));
    scene_state=1;CHECK(!mouse_callbacks.eligible(0xFFFF));scene_state=0;
    scene_flags=0x80;CHECK(!mouse_callbacks.eligible(0xFFFF));scene_flags=0;
    player_paused=255;CHECK(!mouse_callbacks.eligible(0xFFFF));player_paused=0;
    player_pointer=0;CHECK(!mouse_callbacks.eligible(0xFFFF));player_pointer=0x80100000;
    const uint64_t t=1000000000;
    PSXModMouseEvent e{sizeof e,PSX_MOD_MOUSE_MOTION,t,10,-8};
    mouse_callbacks.event(&e);
    PSXModMouseOutput out{sizeof out,0,128,128};
    mouse_callbacks.sample(t,&out);CHECK(out.rx<128 && out.ry>128);
    auto first=out;mouse_callbacks.sample(t,&out);CHECK(first.rx==out.rx && first.ry==out.ry);
    e.time_ns=t+1000000;e.dx=-1;e.dy=1;mouse_callbacks.event(&e);
    mouse_callbacks.sample(e.time_ns,&out);CHECK(out.rx>128 && out.ry<128); // reverse immediately
    mouse_callbacks.sample(e.time_ns+100000000,&out);CHECK(out.rx==128 && out.ry==128);
    e.type=PSX_MOD_MOUSE_RESET;mouse_callbacks.event(&e);
    mouse_callbacks.sample(t,&out);CHECK(out.rx==128 && out.ry==128);
    std::memset(keys,0,sizeof keys);keys[SDL_SCANCODE_W]=keys[SDL_SCANCODE_D]=1;
    pad.buttons=0xFFFF;pad.lx=pad.ly=128;pad.rx=26;
    keyboard_actions(keys,true,&pad);
    CHECK(pad.buttons==0xFFFF && pad.lx==218 && pad.ly==38 && pad.rx==26); // walking and camera coexist
    pad.buttons=0xFFFF;pad.lx=pad.ly=128;keyboard_actions(keys,false,&pad);
    CHECK(pad.buttons==0xFFCF && pad.lx==128 && pad.ly==128);
    keys[SDL_SCANCODE_A]=keys[SDL_SCANCODE_S]=1;
    pad.buttons=0xFFFF;keyboard_actions(keys,true,&pad);CHECK(pad.buttons==0xFFFF);
    const uint32_t defaults[]={0x4000,1,0x8000,0x8400,0x400};
    for(unsigned i=0;i<5;++i) {
        pad.buttons=0xFFFF;mouse_buttons(1u<<i,&pad);CHECK(pad.buttons==(0xFFFFu&~defaults[i]));
        suppressed_button=i+1;pad.buttons=0xFFFF;mouse_buttons(1u<<i,&pad);CHECK(pad.buttons==0xFFFF);suppressed_button=0;
    }
    mouse_actions[0]=action_mask("jump");pad.buttons=0xFFFF;mouse_buttons(1,&pad);CHECK(pad.buttons==0xDFFF);
    CHECK(action_mask("none")==0 && action_mask("invalid")==0);
    // A steady 125Hz stream converges without the old 16ms accumulator resets.
    e.type=PSX_MOD_MOUSE_RESET;event(&e);e.type=PSX_MOD_MOUSE_MOTION;e.dx=2;e.dy=0;
    uint32_t previous=128;
    for(unsigned i=0;i<30;++i) {e.time_ns=t+i*8000000;event(&e);sample(e.time_ns,&out);CHECK(out.rx<=previous);previous=out.rx;}
    CHECK(previous>37 && previous<91);
    camera_start();CHECK(pad_callbacks.transform==transform_camera); // KB/M disabled next session
    puts("Modern controls: simultaneous movement/look, controller defaults, mouse actions, capture policy and replay preservation passed");
}
