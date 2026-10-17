#include "mod_plugins.h"
#include "psx_keybinds.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
uint64_t motion_time;
double mouse_x, mouse_y;
double sensitivity = 1.0;
bool invert_x, invert_y;
bool gameplay() {
    // Retail main-loop entry and persistent menu state, not a host Start toggle.
    if (psx_mod_read_word(0x8004FB9Cu) != 0x8F840584u ||
        psx_mod_read_word(0x800F0604u) != 0 ||
        (psx_mod_read_word(0x800EFF08u) & 0x3FFu) != 0) return false;
    const uint32_t player = psx_mod_read_word(0x800EEFD8u);
    return player >= 0x80010000u && player <= 0x801FFE00u &&
        static_cast<int8_t>(psx_mod_read_byte(player + 0x3B)) >= 0;
}
int eligible(uint32_t) { return gameplay(); }
void event(const PSXModMouseEvent* e) {
    if (e->type == PSX_MOD_MOUSE_RESET || e->type == PSX_MOD_MOUSE_ACQUIRED) {
        mouse_x = mouse_y = 0; motion_time = 0;
    } else if (e->type == PSX_MOD_MOUSE_MOTION) {
        const bool fresh = motion_time && e->time_ns >= motion_time &&
            e->time_ns - motion_time < 16000000;
        mouse_x = std::clamp((fresh && mouse_x * e->dx > 0 ? mouse_x : 0.0) + e->dx, -256.0, 256.0);
        mouse_y = std::clamp((fresh && mouse_y * e->dy > 0 ? mouse_y : 0.0) + e->dy, -256.0, 256.0);
        motion_time = e->time_ns;
    }
}
uint32_t axis(double delta, double fade) {
    if (std::abs(delta) < 0.05 || fade <= 0) return 128;
    const double magnitude = std::clamp(36.0 + std::abs(delta) * sensitivity * 4.0, 0.0, 127.0);
    return static_cast<uint32_t>(std::clamp(std::lround(128.0 + std::copysign(magnitude * fade, delta)), 0l, 255l));
}
void sample(uint64_t now, PSXModMouseOutput* out) {
    out->override_right = 1;
    out->rx = out->ry = 128;
    if (!motion_time || now < motion_time || now - motion_time >= 50000000) return;
    const double fade = 1.0 - static_cast<double>(now - motion_time) / 50000000.0;
    out->rx = axis(invert_x ? -mouse_x : mouse_x, fade);
    out->ry = axis(invert_y ? -mouse_y : mouse_y, fade);
}
uint32_t reverse_axis(uint32_t value) { return std::min(255u, 256u - value); }
void camera_direction(PSXModPadOutput* out) {
    if (invert_x) {
        out->rx = reverse_axis(out->rx);
        const bool left = !(out->buttons & 0x0100), right = !(out->buttons & 0x0200);
        if (left != right) out->buttons ^= 0x0300;
    }
    if (invert_y) out->ry = reverse_axis(out->ry);
}
void keyboard_actions(const uint8_t* keys, bool in_game, PSXModPadOutput* out) {
    // Native actions: no inventory grants, cooldown bypass or direct RAM writes.
    const struct { SDL_Scancode key; uint32_t mask; } actions[] = {
        {SDL_SCANCODE_1, 0x4000}, // primary attack / interact (Cross)
        {SDL_SCANCODE_2, 0x8000}, // secondary / charge (Square)
        {SDL_SCANCODE_3, 0x1000}, // defend / dash (Triangle)
        {SDL_SCANCODE_4, 0x8400}, // switch equipped weapon (L1 + Square)
        {SDL_SCANCODE_5, 0x1400}, // switch Dan / Dan-Hand (L1 + Triangle)
        {SDL_SCANCODE_6, 0x0001}, // inventory / native item use
        {SDL_SCANCODE_E, 0x8400},
        {SDL_SCANCODE_F, 0x4000}, // interact / confirm without a mouse click
        {SDL_SCANCODE_F4, 0x0006}, // L3 + R3: mark a diagnostic incident
    };
    for (const auto& action : actions) if (keys[action.key]) out->buttons &= ~action.mask;
    // Hold Alt for the original first-person look mode; mouse supplies both axes.
    if (in_game && keys[SDL_SCANCODE_LALT]) out->buttons &= ~0x0300u;
    if (in_game) camera_direction(out);
}
int transform(const PSXModPadFrame*, PSXModPadOutput* out) {
    if (!SDL_GetKeyboardFocus()) return 0;
    keyboard_actions(SDL_GetKeyboardState(nullptr), gameplay(), out);
    return 1;
}
void activate() {
    motion_time = 0; mouse_x = mouse_y = 0;
    sensitivity = 1.0;
    char value[32]{};
    if (psx_mod_current_option_value("sensitivity", value, sizeof value)) {
        if (value[0] == 'l') sensitivity = 0.5;
        else if (value[0] == 'h') sensitivity = 2.0;
        else sensitivity = 1.0;
    }
    invert_x = psx_mod_current_option_value("invert-x", value, sizeof value) && value[0] == 't';
    invert_y = psx_mod_current_option_value("invert-y", value, sizeof value) && value[0] == 't';
    PsxPlayerBinds profile{};
    profile.up = profile.ls_up = SDL_SCANCODE_W;
    profile.down = profile.ls_down = SDL_SCANCODE_S;
    profile.left = profile.ls_left = SDL_SCANCODE_A;
    profile.right = profile.ls_right = SDL_SCANCODE_D;
    profile.cross = PSXKB_MOUSE_SC(SDL_BUTTON_LEFT);
    profile.square = PSXKB_MOUSE_SC(SDL_BUTTON_RIGHT);
    profile.circle = SDL_SCANCODE_SPACE;
    profile.triangle = SDL_SCANCODE_LSHIFT;
    profile.l1 = SDL_SCANCODE_Q;
    profile.r1 = SDL_SCANCODE_LCTRL;
    profile.l2 = SDL_SCANCODE_Z;
    profile.r2 = SDL_SCANCODE_C;
    profile.select = SDL_SCANCODE_TAB;
    profile.start = SDL_SCANCODE_RETURN;
    psx_keybinds_set_session_player(1, &profile);
    psx_mod_set_controller_mode_override(0, PSX_MOD_CONTROLLER_ANALOG);
    const PSXModPadTransform pad{sizeof pad,
        PSX_MOD_PAD_TYPE_BIT(PSX_MOD_PAD_DUALSHOCK) | PSX_MOD_PAD_TYPE_BIT(PSX_MOD_PAD_DIGITAL),
        PSX_MOD_PAD_DUALSHOCK, transform};
    psx_mod_set_pad_transform(0, &pad);
    const PSXModMousePolicy mouse{sizeof mouse, PSX_MOD_MOUSE_HOLD_NONE, eligible, event, sample};
    if (psx_mod_set_local_mouse_policy(&mouse)) psx_mod_set_local_mouse_persistent_capture(1);
}
}
PSX_MOD_CONSTRUCTOR(medievil2_register_modern_controls) {
    psx_mod_register_activation_plugin("medievil2.modern-controls", activate);
}
