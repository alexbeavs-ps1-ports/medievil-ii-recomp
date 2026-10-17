#include "mod_plugins.h"
#include "psx_keybinds.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace {
uint64_t motion_time;
double mouse_x, mouse_y, sensitivity = 1.0;
bool invert_x, invert_y, controller_invert_x, controller_invert_y;
uint32_t mouse_actions[5];
bool gameplay() {
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
        // Estimate velocity rather than summing an arbitrary 16ms event batch.
        // A 24ms low-pass removes USB/report-rate steps; reversal stays immediate.
        const double dt = motion_time && e->time_ns >= motion_time
            ? std::clamp(double(e->time_ns - motion_time) / 1e6, 1.0, 100.0) : 8.0;
        const double weight = 1.0 - std::exp(-dt / 24.0);
        auto filter = [dt, weight](double old, double delta) {
            if (old * delta < 0 || dt >= 100) old = 0;
            return old + (std::clamp(delta * 16.667 / dt, -256.0, 256.0) - old) * weight;
        };
        mouse_x = filter(mouse_x, e->dx); mouse_y = filter(mouse_y, e->dy);
        motion_time = e->time_ns;
    }
}
uint32_t axis(double delta, double fade) {
    if (std::abs(delta) < 0.05 || fade <= 0) return 128;
    // Retail normalizes the stick with a 36-unit dead zone (8005FC98).
    const double magnitude = (37.0 + 90.0 * std::tanh(std::abs(delta) * sensitivity / 20.0)) * fade;
    return uint32_t(std::clamp(std::lround(128.0 + std::copysign(magnitude, delta)), 0l, 255l));
}
void sample(uint64_t now, PSXModMouseOutput* out) {
    out->override_right = 1; out->rx = out->ry = 128;
    if (!motion_time || now < motion_time || now - motion_time >= 100000000) return;
    const double fade = std::exp(-double(now - motion_time) / 30000000.0);
    // Native orbit axes turn the view opposite to modern look direction.
    out->rx = axis(invert_x ? mouse_x : -mouse_x, fade);
    out->ry = axis(invert_y ? mouse_y : -mouse_y, fade);
}
uint32_t reverse_axis(uint32_t value) { return std::min(255u, 256u - value); }
void movement(const uint8_t* keys, bool in_game, PSXModPadOutput* out) {
    // Exactly one native movement path. Left stick in gameplay, D-pad in menus.
    const int x = int(keys[SDL_SCANCODE_D] != 0) - int(keys[SDL_SCANCODE_A] != 0);
    const int y = int(keys[SDL_SCANCODE_S] != 0) - int(keys[SDL_SCANCODE_W] != 0);
    if (in_game) {
        const int radius = x && y ? 90 : 127;
        if (x || y) { out->lx = 128 + x * radius; out->ly = 128 + y * radius; }
    } else {
        if (x < 0) out->buttons &= ~0x0080u;
        if (x > 0) out->buttons &= ~0x0020u;
        if (y < 0) out->buttons &= ~0x0010u;
        if (y > 0) out->buttons &= ~0x0040u;
    }
}
void keyboard_actions(const uint8_t* keys, bool in_game, PSXModPadOutput* out) {
    const struct { SDL_Scancode key; uint32_t mask; } actions[] = {
        {SDL_SCANCODE_1, 0x4000}, {SDL_SCANCODE_2, 0x8000},
        {SDL_SCANCODE_3, 0x1000}, {SDL_SCANCODE_4, 0x8400},
        {SDL_SCANCODE_5, 0x1400}, {SDL_SCANCODE_6, 0x0001},
        {SDL_SCANCODE_E, 0x8400}, {SDL_SCANCODE_F, 0x4000},
        {SDL_SCANCODE_F4, 0x0006},
    };
    movement(keys, in_game, out);
    for (const auto& action : actions) if (keys[action.key]) out->buttons &= ~action.mask;
    if (in_game && keys[SDL_SCANCODE_LALT]) out->buttons &= ~0x0300u;
}
void mouse_buttons(uint32_t pressed, PSXModPadOutput* out) {
    for (unsigned i = 0; i < 5; ++i)
        if ((pressed & (1u << i)) &&
            !psx_keybinds_control_suppressed(1, PSXKB_MOUSE_SC(i + 1)))
            out->buttons &= ~mouse_actions[i];
}
int transform_common(const PSXModPadFrame* frame, PSXModPadOutput* out, bool keyboard) {
    // Recordings contain the accepted, already transformed pad.
    if (frame->host_flags & PSX_MOD_PAD_HOST_SOURCE) return 0;
    out->type = PSX_MOD_PAD_DUALSHOCK;
    out->buttons = frame->buttons;
    out->lx = frame->lx; out->ly = frame->ly;
    out->rx = frame->rx; out->ry = frame->ry;
    const bool in_game = gameplay();
    if (in_game) {
        if (!controller_invert_x) out->rx = reverse_axis(out->rx);
        if (!controller_invert_y) out->ry = reverse_axis(out->ry);
    }
    if (keyboard && SDL_GetKeyboardFocus()) {
        keyboard_actions(SDL_GetKeyboardState(nullptr), in_game, out);
        mouse_buttons(SDL_GetMouseState(nullptr, nullptr), out);
    }
    return 1;
}
int transform(const PSXModPadFrame* frame, PSXModPadOutput* out) { return transform_common(frame, out, true); }
int transform_camera(const PSXModPadFrame* frame, PSXModPadOutput* out) { return transform_common(frame, out, false); }
bool option_bool(const char* name) {
    char value[32]{};
    return psx_mod_current_option_value(name, value, sizeof value) && !std::strcmp(value, "true");
}
uint32_t action_mask(const char* name) {
    const struct { const char* name; uint32_t mask; } actions[] = {
        {"attack",0x4000}, {"secondary",0x8000}, {"jump",0x2000},
        {"defend",0x1000}, {"weapon",0x8400}, {"hand",0x1400},
        {"target",0x0400}, {"strafe",0x0800}, {"inventory",0x0001}, {"pause",0x0008}
    };
    for (const auto& action : actions) if (!std::strcmp(name, action.name)) return action.mask;
    return 0;
}
void install_transform(bool keyboard) {
    const PSXModPadTransform pad{sizeof pad,
        PSX_MOD_PAD_TYPE_BIT(PSX_MOD_PAD_DUALSHOCK) | PSX_MOD_PAD_TYPE_BIT(PSX_MOD_PAD_DIGITAL),
        PSX_MOD_PAD_DUALSHOCK, keyboard ? transform : transform_camera};
    psx_mod_set_pad_transform(0, &pad);
}
void activate_camera() {
    controller_invert_x = option_bool("controller-reverse-x");
    controller_invert_y = option_bool("controller-reverse-y");
    install_transform(false);
}
void activate() {
    motion_time = 0; mouse_x = mouse_y = 0;
    sensitivity = 1.0;
    // Read controller options explicitly so activation does not depend on
    // retained globals from a previous session with another feature selection.
    char controller_value[32]{};
    controller_invert_x = psx_mod_option_value("medievil2.input.modern-controls", "camera-controls", "controller-reverse-x", controller_value, sizeof controller_value) && !std::strcmp(controller_value, "true");
    controller_invert_y = psx_mod_option_value("medievil2.input.modern-controls", "camera-controls", "controller-reverse-y", controller_value, sizeof controller_value) && !std::strcmp(controller_value, "true");
    char value[32]{};
    if (psx_mod_current_option_value("sensitivity", value, sizeof value)) {
        if (value[0] == 'l') sensitivity = 0.5;
        else if (value[0] == 'h') sensitivity = 2.0;
    }
    // New option IDs intentionally reset the old native-axis inversion defaults.
    invert_x = option_bool("mouse-reverse-x"); invert_y = option_bool("mouse-reverse-y");
    const char* names[] = {"mouse-left", "mouse-middle", "mouse-right", "mouse-x1", "mouse-x2"};
    const char* defaults[] = {"attack", "inventory", "secondary", "weapon", "target"};
    for (unsigned i=0; i<5; ++i)
        mouse_actions[i] = action_mask(psx_mod_current_option_value(names[i], value, sizeof value) ? value : defaults[i]);
    PsxPlayerBinds profile{};
    // WASD and mouse actions are folded by the transform, never duplicated.
    profile.circle = SDL_SCANCODE_SPACE; profile.triangle = SDL_SCANCODE_LSHIFT;
    profile.l1 = SDL_SCANCODE_Q; profile.r1 = SDL_SCANCODE_LCTRL;
    profile.l2 = SDL_SCANCODE_Z; profile.r2 = SDL_SCANCODE_C;
    profile.select = SDL_SCANCODE_TAB; profile.start = SDL_SCANCODE_RETURN;
    psx_keybinds_set_session_player(1, &profile);
    psx_mod_set_controller_mode_override(0, PSX_MOD_CONTROLLER_ANALOG);
    install_transform(true);
    const PSXModMousePolicy mouse{sizeof mouse, PSX_MOD_MOUSE_HOLD_NONE, eligible, event, sample};
    if (psx_mod_set_local_mouse_policy(&mouse)) {
        psx_mod_set_local_mouse_persistent_capture(1);
        psx_mod_set_local_mouse_capture_across_transitions(1);
    }
}
}
PSX_MOD_CONSTRUCTOR(medievil2_register_modern_controls) {
    psx_mod_register_activation_plugin("medievil2.camera-controls", activate_camera);
    psx_mod_register_activation_plugin("medievil2.modern-controls", activate);
}
