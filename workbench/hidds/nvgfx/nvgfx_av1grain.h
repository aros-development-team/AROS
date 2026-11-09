/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AV1 film grain synthesis for NVDEC: the driver generates the grain
          templates and scaling tables (AV1FilmGrainMemory); the engine adds
          them to the picture. Plain C so that it also builds on a host for
          testing against the memory NVIDIA's driver writes.
*/
#ifndef NOUVEAU_AV1GRAIN_H
#define NOUVEAU_AV1GRAIN_H

#include <stdint.h>

#define AV1_GRAIN_MEMORY_SIZE   (3 * 256 + 2 * (4096 + 2 * 1024))  /* AV1FilmGrainMemory */

struct av1_grain_params
{
    uint16_t seed;
    int bit_depth;                  /* 8 or 10 */
    int num_y, num_cb, num_cr;
    uint8_t y_value[14], y_scaling[14];
    uint8_t cb_value[10], cb_scaling[10];
    uint8_t cr_value[10], cr_scaling[10];
    int chroma_scaling_from_luma;
    int ar_coeff_lag;               /* 0..3 */
    int ar_coeff_shift;             /* 6..9 */
    int grain_scale_shift;          /* 0..3 */
    int8_t ar_y[24], ar_cb[25], ar_cr[25];
};

/* fill the AV1FilmGrainMemory layout: scaling LUTs y/cb/cr, then the 64x64 luma and 32x32 cb/cr grain blocks */
void av1_grain_generate(const struct av1_grain_params *p, uint8_t *out);

#endif
