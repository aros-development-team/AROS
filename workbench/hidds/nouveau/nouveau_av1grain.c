/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AV1 film grain synthesis (AV1 specification 7.18.3.3 generate
          grain, 7.18.3.4 scaling lookup), producing the AV1FilmGrainMemory
          the NVDEC engine reads: the 73x82 luma and 38x44 chroma templates
          (4:2:0) after the auto-regressive filter, cropped to the 64x64 and
          32x32 parts the per-pixel stage samples from. Checked byte for byte
          against the memory NVIDIA's driver writes.
*/

#include <string.h>
#include "nouveau_av1grain.h"
#include "nouveau_av1grain_tab.h"

#define LUMA_H      73
#define LUMA_W      82
#define CHROMA_H    38
#define CHROMA_W    44

struct grain_rng
{
    uint16_t r;
};

static int grain_rand(struct grain_rng *g, int bits)
{
    unsigned int r = g->r, bit = ((r >> 0) ^ (r >> 1) ^ (r >> 3) ^ (r >> 12)) & 1;

    r = ((r >> 1) | (bit << 15)) & 0xffff;
    g->r = (uint16_t)r;
    return (int)((r >> (16 - bits)) & ((1u << bits) - 1));
}

static void grain_rng_line(struct grain_rng *g, int line, uint16_t seed)
{
    int n = line >> 5;

    g->r = seed;
    g->r ^= (uint16_t)(((n * 37 + 178) & 255) << 8);
    g->r ^= (uint16_t)((n * 173 + 105) & 255);
}

static int clampi(int v, int lo, int hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static void scaling_lut(const uint8_t *value, const uint8_t *scaling, int n, uint8_t *lut)
{
    int i, a, x;

    memset(lut, 0, 256);
    if (!n)
        return;
    for (i = 0; i < value[0]; i++)
        lut[i] = scaling[0];
    for (a = 0; a < n - 1; a++)
    {
        int dy = scaling[a + 1] - scaling[a], dx = value[a + 1] - value[a];
        int64_t d = (int64_t)dy * ((65536 + (dx >> 1)) / dx);

        for (x = 0; x < dx; x++)
            lut[value[a] + x] = (uint8_t)(scaling[a] + (int)((x * d + 32768) >> 16));
    }
    for (i = value[n - 1]; i < 256; i++)
        lut[i] = scaling[n - 1];
}

static void put16(uint8_t *o, int v)
{
    o[0] = (uint8_t)v;
    o[1] = (uint8_t)(v >> 8);
}

void av1_grain_generate(const struct av1_grain_params *p, uint8_t *out)
{
    static int16_t L[LUMA_H][LUMA_W], C[2][CHROMA_H][CHROMA_W];
    struct grain_rng g;
    int shift = 12 - p->bit_depth + p->grain_scale_shift, round = (1 << shift) >> 1;
    int gmin = -(128 << (p->bit_depth - 8)), gmax = (128 << (p->bit_depth - 8)) - 1;
    int lag = p->ar_coeff_lag, npos = 2 * lag * (lag + 1), rs = p->ar_coeff_shift, rround = 1 << (rs - 1);
    int i, j, q;
    const int8_t *arc[2] = { p->ar_cb, p->ar_cr };
    int numc[2] = { p->num_cb, p->num_cr };
    uint16_t lines[2] = { 7 << 5, 11 << 5 };

    memset(L, 0, sizeof(L));
    memset(C, 0, sizeof(C));
    if (p->num_y)
    {
        g.r = p->seed;
        for (i = 0; i < LUMA_H; i++)
            for (j = 0; j < LUMA_W; j++)
                L[i][j] = (int16_t)((av1_gaussian_sequence[grain_rand(&g, 11)] + round) >> shift);
        for (i = 3; i < LUMA_H; i++)
            for (j = 3; j < LUMA_W - 3; j++)
            {
                int w = 0, pos = 0, dr, dc;

                for (dr = -lag; dr <= 0; dr++)
                    for (dc = -lag; dc <= lag && pos < npos; dc++, pos++)
                        w += p->ar_y[pos] * L[i + dr][j + dc];
                L[i][j] = (int16_t)clampi(L[i][j] + ((w + rround) >> rs), gmin, gmax);
            }
    }
    for (q = 0; q < 2; q++)
        if (numc[q] || p->chroma_scaling_from_luma)
        {
            grain_rng_line(&g, lines[q], p->seed);
            for (i = 0; i < CHROMA_H; i++)
                for (j = 0; j < CHROMA_W; j++)
                    C[q][i][j] = (int16_t)((av1_gaussian_sequence[grain_rand(&g, 11)] + round) >> shift);
        }
    for (i = 3; i < CHROMA_H; i++)
        for (j = 3; j < CHROMA_W - 3; j++)
        {
            int w[2] = { 0, 0 }, pos = 0, dr, dc;

            for (dr = -lag; dr <= 0; dr++)
                for (dc = -lag; dc <= lag && pos < npos; dc++, pos++)
                    for (q = 0; q < 2; q++)
                        w[q] += arc[q][pos] * C[q][i + dr][j + dc];
            if (p->num_y)
            {
                /* the co-located luma (average of the 2x2 block) is the last coefficient */
                int ly = ((i - 3) << 1) + 3, lx = ((j - 3) << 1) + 3;
                int av = (L[ly][lx] + L[ly][lx + 1] + L[ly + 1][lx] + L[ly + 1][lx + 1] + 2) >> 2;

                for (q = 0; q < 2; q++)
                    w[q] += arc[q][npos] * av;
            }
            for (q = 0; q < 2; q++)
                if (numc[q] || p->chroma_scaling_from_luma)
                    C[q][i][j] = (int16_t)clampi(C[q][i][j] + ((w[q] + rround) >> rs), gmin, gmax);
        }

    scaling_lut(p->y_value, p->y_scaling, p->num_y, out);
    if (p->chroma_scaling_from_luma)
    {
        memcpy(out + 256, out, 256);
        memcpy(out + 512, out, 256);
    }
    else
    {
        scaling_lut(p->cb_value, p->cb_scaling, p->num_cb, out + 256);
        scaling_lut(p->cr_value, p->cr_scaling, p->num_cr, out + 512);
    }
    out += 768;
    for (i = 0; i < 64; i++)
        for (j = 0; j < 64; j++, out += 2)
            put16(out, L[9 + i][9 + j]);
    for (q = 0; q < 2; q++)
        for (i = 0; i < 32; i++)
            for (j = 0; j < 32; j++, out += 2)
                put16(out, C[q][6 + i][6 + j]);
}
