#include "medievil2_seamless_store.h"
#include "mod_plugins.h"
#include "bios_hle.h"
#include "cpu_state.h"
#include "dirty_ram_interp.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t *memory_get_ram_ptr(void);
static int ready,trace;
static uint32_t completion;
static int (*previous_hook)(CPUState *,uint32_t);
static uint32_t r32(uint32_t address) { return psx_mod_read_word(address); }
static void w32(uint32_t address,uint32_t value) { psx_mod_write_word(address,value); }
static int ram(uint32_t address,uint32_t bytes) {
    return address>=0x80010000u && address<0x80200000u && bytes<=0x80200000u-address;
}
static int heap(uint32_t address,uint32_t bytes) {
    return address>=0x800EF000u && ram(address,bytes);
}
static int guard(uint32_t address,uint32_t first,uint32_t second) {
    return r32(address)==first && r32(address+4)==second;
}
static void copy_ram(uint32_t dest,const unsigned char *source,uint32_t bytes) {
    memcpy(memory_get_ram_ptr()+(dest&0x1fffffffu),source,bytes);
    dirty_ram_mark_executable_range(dest&0x1fffffffu,bytes);
    // Publish host writes to overlay page generations and memory observers.
    for(uint32_t p=dest;p<dest+bytes;p=(p&~255u)+256) psx_mod_write_byte(p,source[p-dest]);
}
static int bcd(unsigned value,unsigned maximum) {
    if((value&15)>9 || (value>>4)>9) return -1;
    unsigned n=10*(value>>4)+(value&15);return n<maximum?(int)n:-1;
}
static int dispatch(CPUState *cpu,uint32_t pc) {
    if(!ready) return previous_hook?previous_hook(cpu,pc):0;
    uint32_t gp=cpu->gpr[28];
    if(!ram(gp,0x880)) return previous_hook?previous_hook(cpu,pc):0;
    if(pc==0xAA244u && cpu->gpr[31]==0x800707C8u &&
       guard(0x800AA244u,0x27BDFFD8u,0xAFB10014u)) {
        uint32_t desc=cpu->gpr[4],dest=cpu->gpr[5];
        if(ram(desc,12)) {
            int m=bcd(psx_mod_read_byte(desc),100),s=bcd(psx_mod_read_byte(desc+1),60),
                f=bcd(psx_mod_read_byte(desc+2),75);
            uint32_t size=r32(desc+4),bytes=(size+2047u)&~2047u;
            int lba=(m*60+s)*75+f-150;
            const unsigned char *source=m>=0 && s>=0 && f>=0 && lba>=0 && size &&
                size<=0x200000u && heap(dest,bytes)?medievil2_seamless_source((uint32_t)lba,bytes):NULL;
            if(source) {
                copy_ram(dest,source,bytes);
                w32(gp+0x848,0);w32(gp+0x84C,0);w32(gp+0x850,0);
                cpu->gpr[2]=0;
                psx_mod_counter_add("medievil2.seamless.module_reads",1);
                if(trace) { fprintf(stdout,"medievil2 seamless: module lba=%d bytes=%u\n",lba,bytes);fflush(stdout); }
                return 1;
            }
        }
        psx_mod_counter_add("medievil2.seamless.module_fallback",1);
    } else if(pc==0xA0654u && cpu->gpr[31]==0x800A0500u && completion &&
              guard(0x800A0654u,0x27BDFFE0u,0x3C06DEADu)) {
        uint32_t index=cpu->gpr[4],table=r32(0x800F4940u);
        if(index<0x51Au && table==0x800BF6A4u && r32(0x800F4938u)==53382u &&
           !r32(completion)) {
            uint32_t entry=table+36*index,dest=r32(entry+0x10),size=r32(entry+0x18),
                sector=r32(entry+0xC),bytes=(size+2047u)&~2047u;
            const unsigned char *source=size && size<=0x200000u && sector<16818u && heap(dest,bytes)?
                medievil2_seamless_source(53382u+sector,bytes):NULL;
            if(source) {
                copy_ram(dest,source,bytes);w32(gp+0x848,0);
                // Completion lives in snapshot-backed Expansion 1 memory,
                // never a host-only pending operation or the game's heap.
                w32(completion+4,index);w32(completion+8,dest);w32(completion+12,gp);
                w32(completion+16,size);w32(completion+20,sector);w32(completion,0x4D324344u);
                cpu->gpr[2]=1;psx_mod_counter_add("medievil2.seamless.archive_reads",1);
                if(trace) { fprintf(stdout,"medievil2 seamless: asset=%u bytes=%u\n",index,bytes);fflush(stdout); }
                return 1;
            }
        }
        psx_mod_counter_add("medievil2.seamless.archive_fallback",1);
    } else if(pc==0xA06F0u && cpu->gpr[31]==0x800A0518u && completion &&
              guard(0x800A06F0u,0x27BDFFE0u,0x3C03800Fu) &&
              r32(completion)==0x4D324344u && r32(completion+4)==cpu->gpr[4] &&
              r32(completion+12)==gp && r32(0x800F4940u)==0x800BF6A4u) {
        uint32_t entry=0x800BF6A4u+36*cpu->gpr[4];
        if(cpu->gpr[4]<0x51Au && r32(entry+0x10)==r32(completion+8) &&
           r32(entry+0x18)==r32(completion+16) && r32(entry+0xC)==r32(completion+20)) {
            w32(completion,0);w32(gp+0x84C,0);cpu->gpr[2]=0;
            psx_mod_counter_add("medievil2.seamless.completions",1);return 1;
        }
    } else if(pc==0xADFB4u && cpu->gpr[31]==0x800A07FCu &&
              guard(0x800ADFB4u,0x27BDFFD0u,0x00803821u)) {
        uint32_t src=cpu->gpr[4],dest=cpu->gpr[5],size=cpu->gpr[6];
        if(size && heap(src,size)) {
            uint32_t length,prefix,cursor,remaining,bits;
            const unsigned char *decoded=medievil2_seamless_decoded(
                memory_get_ram_ptr()+(src&0x1fffffffu),size,&length,&prefix,&cursor,&remaining,&bits);
            if(decoded && dest==src+prefix && heap(dest,length)) {
                copy_ram(dest,decoded,length);
                w32(gp+0x86C,src+cursor);w32(gp+0x870,remaining);w32(gp+0x874,bits);
                cpu->gpr[2]=1;psx_mod_counter_add("medievil2.seamless.predecoded",1);return 1;
            }
        }
        psx_mod_counter_add("medievil2.seamless.decode_fallback",1);
    }
    return previous_hook?previous_hook(cpu,pc):0;
}
static void tick(void) {
    if(ready && g_psx_bios_hle_hook!=dispatch) {
        previous_hook=g_psx_bios_hle_hook;g_psx_bios_hle_hook=dispatch;
    }
}
static void activate(void) {
    const char *value=getenv("MEDIEVIL2_SEAMLESS_TRACE");trace=value && !strcmp(value,"1");
    ready=medievil2_seamless_prepare();
    if(ready) { completion=psx_mod_alloc_guest_memory(24,4);if(!completion) ready=0; }
}
PSX_MOD_CONSTRUCTOR(medievil2_register_seamless) {
    (void)psx_mod_register_activation_plugin("medievil2.seamless",activate);
    (void)psx_mod_register_vblank_plugin("medievil2.seamless",tick);
}
