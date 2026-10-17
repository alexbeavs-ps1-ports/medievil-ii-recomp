#include "../src/mods/medievil2_modern_controls.cpp"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#expr); std::exit(1); } } while (0)
static PSXModActivationCallback start;
static PsxPlayerBinds bindings;
static PSXModMousePolicy mouse_callbacks;
static uint32_t scene_state, scene_flags, player_pointer = 0x80100000;
static uint8_t player_paused;
static int persistent, analog;
extern "C" {
int psx_mod_register_activation_plugin(const char*, PSXModActivationCallback cb) { start=cb;return 1; }
uint32_t psx_mod_read_word(uint32_t a) {
    if(a==0x8004FB9C)return 0x8F840584;
    if(a==0x800F0604)return scene_state;
    if(a==0x800EFF08)return scene_flags;
    if(a==0x800EEFD8)return player_pointer;
    return 0;
}
uint8_t psx_mod_read_byte(uint32_t) {return player_paused;}
int psx_mod_current_option_value(const char* name,char* out,uint32_t cap) {
    std::snprintf(out,cap,"%s",!std::strcmp(name,"invert-y")?"false":"normal");return 1;
}
int psx_keybinds_set_session_player(int player,const PsxPlayerBinds* p) {CHECK(player==1);bindings=*p;return 1;}
int psx_mod_set_controller_mode_override(uint32_t player,uint32_t mode) {CHECK(player==0);analog=mode;return 1;}
int psx_mod_set_pad_transform(uint32_t,const PSXModPadTransform*) {return 1;}
int psx_mod_set_local_mouse_policy(const PSXModMousePolicy* p) {mouse_callbacks=*p;return 1;}
int psx_mod_set_local_mouse_persistent_capture(int on) {persistent=on;return 1;}
}
int main() {
    CHECK(start);start();
    CHECK(persistent && analog==PSX_MOD_CONTROLLER_ANALOG);
    CHECK(bindings.circle==SDL_SCANCODE_SPACE && bindings.cross==PSXKB_MOUSE_SC(SDL_BUTTON_LEFT));
    CHECK(bindings.ls_up==SDL_SCANCODE_W && bindings.r1==SDL_SCANCODE_LCTRL);
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
    CHECK(mouse_callbacks.eligible(0xFFFF));
    scene_state=1;CHECK(!mouse_callbacks.eligible(0xFFFF));scene_state=0;
    scene_flags=0x80;CHECK(!mouse_callbacks.eligible(0xFFFF));scene_flags=0;
    player_paused=255;CHECK(!mouse_callbacks.eligible(0xFFFF));player_paused=0;
    player_pointer=0;CHECK(!mouse_callbacks.eligible(0xFFFF));player_pointer=0x80100000;
    const uint64_t t=1000000000;
    PSXModMouseEvent e{sizeof e,PSX_MOD_MOUSE_MOTION,t,10,-8};
    mouse_callbacks.event(&e);
    PSXModMouseOutput out{sizeof out,0,128,128};
    mouse_callbacks.sample(t,&out);CHECK(out.rx>128 && out.ry<128);
    auto first=out;mouse_callbacks.sample(t,&out);CHECK(first.rx==out.rx && first.ry==out.ry);
    e.time_ns=t+1000000;e.dx=-1;e.dy=1;mouse_callbacks.event(&e);
    mouse_callbacks.sample(e.time_ns,&out);CHECK(out.rx<128 && out.ry>128); // reverse immediately
    mouse_callbacks.sample(e.time_ns+50000000,&out);CHECK(out.rx==128 && out.ry==128);
    e.type=PSX_MOD_MOUSE_RESET;mouse_callbacks.event(&e);
    mouse_callbacks.sample(t,&out);CHECK(out.rx==128 && out.ry==128);
    invert_x=invert_y=true;
    PSXModPadOutput pad{};pad.buttons=0xFEFF;pad.rx=230;pad.ry=40;
    camera_direction(&pad);
    CHECK(pad.rx<128 && pad.ry>128 && pad.buttons==0xFDFF);
    pad.buttons=0xFCFF;pad.rx=pad.ry=128;camera_direction(&pad);
    CHECK(pad.rx==128 && pad.ry==128 && pad.buttons==0xFCFF); // keep first-person chord
    puts("Modern controls: native bindings, game-state gates, reversal, decay and non-consuming samples passed");
}
