#include "mod_plugins.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* The framework renderer recovers precise horizontal projection from packet
 * provenance. Guest GTE registers, gameplay and FMV proportions stay original.
 * Game-owned terrain and culling adapters are bound separately by byte guards. */
static void activate(void) {
    char view[24];
    if (!psx_mod_set_main_ram_8mb(1)) {
        fprintf(stderr, "MediEvil II: expanded render memory unavailable\n");
        abort();
    }
    if (!psx_mod_option_value("medievil2.enhancement.widescreen", "widescreen",
                              "aspect", view, sizeof view))
        strcpy(view, "Fit");
    psx_mod_set_native_wide_projection_correction(1);
    psx_mod_set_native_wide_near_clip(1);
    const uint32_t sites[] = {0x8007FCE0u, 0x8007FCE8u, 0x8007FD18u};
    const uint32_t words[] = {0x1F000003u, 0x07210255u, 0x1BC00249u};
    psx_mod_set_native_wide_nclip_sites(sites, words, 3);
    /* The quad funnel saves MAC0 for both triangles before testing either.
     * Its first branch at FCE0 therefore consumes the preceding NCLIP, while
     * FCE8 and the single-triangle branch consume the latest command. */
    psx_mod_set_native_wide_nclip_previous_site(sites[0], words[0]);
    if (!strcmp(view, "4:3")) {
        (void)psx_mod_set_fixed_display_aspect(4, 3);
        return;
    }
    unsigned numerator = !strcmp(view, "21:9") ? 21u :
                         !strcmp(view, "32:9") ? 32u : 16u;
    (void)psx_mod_set_fixed_display_aspect(numerator, 9);
    if (!strcmp(view, "Fit"))
        (void)psx_mod_set_adaptive_display_aspect(0, 0);
}

PSX_MOD_CONSTRUCTOR(medievil2_register_widescreen) {
    (void)psx_mod_register_activation_plugin("medievil2.widescreen", activate);
}
