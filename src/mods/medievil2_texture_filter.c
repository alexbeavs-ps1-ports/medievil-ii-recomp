#include "mod_plugins.h"
static void stable(void) { psx_mod_set_texture_filter(2); }
PSX_MOD_CONSTRUCTOR(register_texture_filter) {
    psx_mod_register_activation_plugin("medievil2.filter.stable",stable);
}
