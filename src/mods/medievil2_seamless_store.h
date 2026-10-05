#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int medievil2_seamless_prepare(void);
const unsigned char *medievil2_seamless_source(uint32_t lba,uint32_t bytes);
const unsigned char *medievil2_seamless_decoded(const unsigned char *source,uint32_t size,
    uint32_t *length,uint32_t *prefix,uint32_t *cursor,uint32_t *remaining,uint32_t *bits);
#ifdef __cplusplus
}
#endif
