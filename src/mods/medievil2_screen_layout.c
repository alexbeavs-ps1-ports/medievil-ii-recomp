#include "mod_plugins.h"
#include "cpu_state.h"
#include "psx_cycle_freeze.h"
#include <math.h>

/* SCUS-94564 MED2.EXE owns the bar and iris packet producers. Record their
 * bounded allocations on entry, then tag completed packets immediately before
 * DrawOTag submits them. Nothing is inferred from a world packet's position.
 * Live and replay submissions own separate pending lists: replay runs inside
 * Swap before the live DrawOTag, with different packet allocations. Consuming
 * its caller's list there would leave the live bars untagged. Renderer tags
 * are word guarded and transient across reset/load. A missing tag leaves native layout. */
enum { SCRATCH_CURSOR=0x1F80006Cu, SCRATCH_END=0x1F800070u,
       PENDING_CAPACITY=512u, PANEL=1, IRIS=2, CENTRED_TEXT=3 };
typedef struct PendingMask { uint32_t packet; unsigned kind; } PendingMask;
static PendingMask pending[2][PENDING_CAPACITY];
static unsigned pending_count[2];

/* Status-panel callbacks run before the replay snapshot at 0x80050044.
 * Their scratch records (including the fade copies) belong to that snapshot
 * and must be tagged again for every replay and the final live submission.
 * Ownership comes from the panel's 14 descriptors, never screen coordinates. */
typedef struct HudRange { uint32_t begin, end; int edge; uint32_t guard; } HudRange;
static HudRange hud_ranges[14], hud_scope;
static unsigned hud_count;
static uint32_t hud_node, hud_arena_end;
static const uint32_t hud_callbacks[14]={
    0x80078AFCu,0x80078AFCu,0x80078B98u,0x80079010u,
    0x8007919Cu,0x80078BF4u,0x80078D74u,0x80079ECCu,
    0x80079264u,0x8007BBC0u,0x8007955Cu,0x8007955Cu,
    0x800791D4u,0x8007921Cu
};
static const int hud_edges[14]={-1,-1,-1,0,0,1,1,0,1,0,-1,1,-1,1};
static int ram_range(uint32_t p,uint32_t bytes) {
    return !(p&3u) && p>=0x80010000u && p<0x80800000u &&
           bytes<=0x80800000u-p;
}
static void clear_hud(void) {
    hud_count=0; hud_node=0; hud_scope=(HudRange){0}; hud_arena_end=0;
}
void medievil2_screen_layout_reset(void) {
    clear_hud(); pending_count[0]=pending_count[1]=0;
}
static void begin_frame(CPUState *cpu,uint32_t address) {
    (void)cpu; (void)address;
    if (!g_psx_render_pass_active) { clear_hud(); psx_mod_counter_add("medievil2.hud.frame",1); }
}
static uint32_t hud_guard(uint32_t begin,uint32_t end) {
    uint32_t hash=2166136261u;
    for (uint32_t p=begin;p<end;p+=4) hash=(hash^psx_mod_read_word(p))*16777619u;
    return hash;
}
static void begin_hud_widget(CPUState *cpu,uint32_t address) {
    (void)address;
    psx_mod_counter_add("medievil2.hud.begin",1);
    hud_node=0;
    if (g_psx_render_pass_active || hud_count>=14) return;
    uint32_t gp=cpu->gpr[28];
    if (!ram_range(gp,0x7B4u)) return;
    uint32_t panel=psx_mod_read_word(gp+0x79Cu);
    uint32_t node=psx_mod_read_word(gp+0x7B0u);
    if (!ram_range(panel,4) || !ram_range(node,28)) return;
    uint32_t first=psx_mod_read_word(panel), index=psx_mod_read_word(node+8);
    if (index>=14 || !ram_range(first,14*28) || node!=first+index*28 ||
        psx_mod_read_word(node+4)!=hud_callbacks[index] ||
        cpu->gpr[2]!=hud_callbacks[index]) return;
    uint32_t p=psx_mod_read_word(SCRATCH_CURSOR), end=psx_mod_read_word(SCRATCH_END);
    if (!ram_range(p,4) || !ram_range(end-4,4) || end<=p) return;
    hud_node=node; hud_arena_end=end;
    hud_scope=(HudRange){p,p,hud_edges[index],0};
    psx_mod_counter_add("medievil2.hud.owned",1);
}
static void end_hud_widget(CPUState *cpu,uint32_t address) {
    (void)address;
    if (!hud_node) return;
    uint32_t node=hud_node;
    hud_node=0;
    if (!ram_range(cpu->gpr[28],0x7B4u) ||
        psx_mod_read_word(cpu->gpr[28]+0x7B0u)!=node ||
        psx_mod_read_word(SCRATCH_END)!=hud_arena_end) return;
    uint32_t end=psx_mod_read_word(SCRATCH_CURSOR);
    if ((end&3u) || end<=hud_scope.begin || end>hud_arena_end ||
        end-hud_scope.begin>0x10000u || hud_count>=14) return;
    hud_scope.end=end;
    hud_scope.guard=hud_guard(hud_scope.begin,end);
    hud_ranges[hud_count++]=hud_scope;
    psx_mod_counter_add("medievil2.hud.range",1);
}
/* MED2's fade-copy routine at 0x8007C37C walks these same five record sizes.
 * Each record has a four-byte type/opacity prefix and one DMA packet.
 * Compound packets contain E1, NOP and a draw command. Anchor the word
 * immediately before that draw command, not the packet's initial DMA tag. */
static int hud_packets(const HudRange *range,int tag) {
    static const unsigned payload_bytes[5]={28,40,36,44,40};
    uint32_t p=range->begin;
    while (p<range->end) {
        unsigned type=psx_mod_read_half(p);
        if (type>=5 || range->end-p<4+payload_bytes[type]) return 0;
        uint32_t next=p+4+payload_bytes[type];
        if ((psx_mod_read_word(p+4)>>24)!=payload_bytes[type]/4-1) return 0;
        for (uint32_t command=p+8; command<next;) {
            uint32_t word=psx_mod_read_word(command);
            unsigned op=word>>24;
            unsigned plain=op&0xFCu;
            unsigned words=plain==0x64 ? 4 : plain==0x28 ? 5 :
                plain==0x2C ? 9 : plain==0x30 ? 6 : plain==0x34 ? 9 :
                plain==0x38 ? 8 : plain==0x3C ? 12 : 0;
            int draw=words!=0;
            if (!draw && (op==0xE1 || word==0)) words=1;
            if (!words || words*4>next-command) return 0;
            if (tag && draw) psx_mod_anchor_hud_primitive(command-4,range->edge);
            command+=4*words;
        }
        p=next;
    }
    return p==range->end;
}
static void submit_hud(void) {
    for (unsigned i=0;i<hud_count;++i) {
        if (hud_guard(hud_ranges[i].begin,hud_ranges[i].end)!=hud_ranges[i].guard)
            psx_mod_counter_add("medievil2.hud.changed",1);
        else if (!hud_packets(&hud_ranges[i],0)) psx_mod_counter_add("medievil2.hud.invalid",1);
        else { hud_packets(&hud_ranges[i],1); psx_mod_counter_add("medievil2.hud.tagged",1); }
    }
}

static void begin_draw(CPUState *cpu, uint32_t address) {
    (void)cpu; (void)address;
    /* Each sandbox starts again from the section-entry RAM. Do not carry
     * allocations from a previous pass, even if its submission was aborted.
     * The live list may already contain masks and must remain untouched. */
    if (g_psx_render_pass_active) pending_count[1]=0;
}

static int guarded(uint32_t p, uint32_t a, uint32_t b) {
    return psx_mod_read_word(p)==a && psx_mod_read_word(p+4)==b;
}
static uint32_t allocation(uint32_t bytes) {
    uint32_t p=psx_mod_read_word(SCRATCH_CURSOR), end=psx_mod_read_word(SCRATCH_END);
    if ((p&3u) || p<0x80010000u || p>=0x80800000u || end>0x80800000u ||
        end<=p || bytes>=end-p) return 0;
    return p;
}
static void remember(uint32_t packet, unsigned kind) {
    unsigned pass=g_psx_render_pass_active!=0;
    if (pending_count[pass]<PENDING_CAPACITY)
        pending[pass][pending_count[pass]++]=(PendingMask){packet,kind};
}
static void prepare_bars(CPUState *cpu, uint32_t address) {
    (void)cpu;
    if (!guarded(address,0x3C02800Fu,0x8C46F384u)) return;
    uint32_t state=psx_mod_read_word(0x800EF384u);
    if (state<0x80010000u || state>0x801FFFF8u ||
        (psx_mod_read_half(state)==240 && psx_mod_read_half(state+2)==240)) return;
    uint32_t p=allocation(48);
    if (p) { remember(p,PANEL); remember(p+24,PANEL); }
}
static void prepare_iris(CPUState *cpu, uint32_t address) {
    (void)cpu;
    if (!guarded(address,0x27BDFFF0u,0xAFB10004u)) return;
    uint32_t p=allocation(0x7A0u);
    if (!p) return;
    /* 32 feathered G4s followed by 32 solid F4s, then the dummy GT3. */
    for (unsigned i=0; i<32; ++i) {
        remember(p+i*36,IRIS);
        remember(p+0x480u+i*24,IRIS);
    }
}
static void prepare_subtitle_glyph(CPUState *cpu, uint32_t address) {
    if (!guarded(address,0x30A500FFu,0x2402000Au)) return;
    unsigned character=cpu->gpr[5]&255u;
    uint32_t gp=cpu->gpr[28];
    if (gp<0x80010000u || gp>0x801FFA00u || character==10 ||
        cpu->gpr[4]!=psx_mod_read_word(gp+0x590u)) return;
    uint32_t p=allocation(24);
    /* The glyph is a compound E1+SPRT packet. Anchor its SPRT command only. */
    if (p) remember(p+4,CENTRED_TEXT);
}
static void submit(CPUState *cpu, uint32_t address) {
    (void)cpu;
    unsigned pass=g_psx_render_pass_active!=0;
    if (!guarded(address,0x3C030400u,0x3C02800Fu)) { pending_count[pass]=0; return; }
    submit_hud();
    unsigned width=psx_mod_display_width(), height=psx_mod_display_height();
    int margin=psx_mod_widescreen_view_x_margin();
    /* This producer uses X radius R and Y radius R/2. Scale both rings
     * together to reach the new corner at the same transition phase. */
    double radius=hypot(width*.5,height);
    float scale=width && height && margin>0 && radius>0 ?
        (float)(hypot(width*.5+margin,height)/radius) : 1.f;
    for (unsigned i=0; i<pending_count[pass]; ++i) {
        uint32_t p=pending[pass][i].packet, tag=psx_mod_read_word(p),
                 command=psx_mod_read_word(p+4);
        if (pending[pass][i].kind==PANEL) {
            if ((tag>>24)==5 && command==0x28000000u)
                psx_mod_tag_screen_mask_quad(p);
        } else if (pending[pass][i].kind==CENTRED_TEXT) {
            if ((command>>24)==0x64u)
                psx_mod_anchor_hud_primitive(p,0);
        } else if (((tag>>24)==5 && command==0x28000000u) ||
                   ((tag>>24)==8 && command==0x3A000000u)) {
            psx_mod_tag_radial_screen_mask_quad(p,scale);
        }
    }
    pending_count[pass]=0;
}
PSX_MOD_CONSTRUCTOR(medievil2_register_screen_layout) {
    const char *plugin="medievil2.widescreen";
    (void)psx_mod_register_savestate_plugin(plugin,medievil2_screen_layout_reset);
    (void)psx_mod_register_function_entry_plugin(plugin,0x8004FFB4u,begin_frame);
    (void)psx_mod_register_instruction_plugin(plugin,0x80078A14u,0x0040F809u,begin_hud_widget);
    (void)psx_mod_register_instruction_plugin(plugin,0x80078A44u,0x8F8207B0u,end_hud_widget);
    (void)psx_mod_register_instruction_plugin(plugin,0x80050044u,0x0C026E80u,begin_draw);
    (void)psx_mod_register_function_entry_plugin(plugin,0x8001A2E4u,prepare_bars);
    (void)psx_mod_register_function_entry_plugin(plugin,0x800456F8u,prepare_iris);
    (void)psx_mod_register_function_entry_plugin(plugin,0x800A4C50u,submit);
    (void)psx_mod_register_function_entry_plugin(plugin,0x8009F7F0u,prepare_subtitle_glyph);
}
