/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include "synth_internal.h"

#define FRAME 8
#define END 0xff
#define HOLE 0xfe

#if defined(__m68k__) && defined(__GNUC__)
# define SC_M68K_HOT __attribute__((optimize("O3,no-strict-aliasing")))
#else
# define SC_M68K_HOT
#endif

static uint32_t InterpolateAttrs(const uint32_t *attrs, uint8_t p)
{
    return p == END ? 0 : attrs[p];
}

static SC_M68K_HOT void InterpolateLine(uint8_t *f, size_t at, unsigned col, unsigned count)
{
    int from = f[at + col], to = f[at + count * FRAME + col];
    int step = ((to - from) * 32) / (int)count;
    int16_t acc = (int16_t)(from << 5);
    unsigned k;
    for (k = 1; k < count; ++k)
    {
        acc = (int16_t)(acc + step);
        f[at + k * FRAME + col] = (uint8_t)(acc >> 5);
    }
}

static SC_M68K_HOT void FillZero(uint8_t *f, unsigned col)
{
    size_t at = 0;
    for (;;)
    {
        unsigned span = 1;
        while (f[at + span * FRAME + col] == 0)
        {
            ++span;
        }
        if (f[at + span * FRAME + col] == END)
        {
            return;
        }
        if (span == 1)
        {
            at += FRAME;
        }
        else
        {
            InterpolateLine(f, at, col, span);
            at += span * FRAME;
        }
    }
}

static SC_M68K_HOT void FillMarked(uint8_t *f, unsigned col)
{
    size_t at = 0;
    for (;;)
    {
        unsigned span = 1;
        for (;; ++span)
        {
            uint8_t v = f[at + span * FRAME + col];
            if (v == END)
            {
                return;
            }
            if (v != HOLE)
            {
                break;
            }
        }
        if (span == 1)
        {
            at += FRAME;
        }
        else
        {
            InterpolateLine(f, at, col, span);
            at += span * FRAME;
        }
    }
}

static SC_M68K_HOT void Smooth7Plain(uint8_t *f, unsigned col)
{
    size_t at = 0;
    for (;; at += FRAME)
    {
        uint16_t sum = 0;
        int k;
        for (k = 6; k >= 0; --k)
        {
            uint8_t v = f[at + k * FRAME + col];
            if (v == END)
            {
                return;
            }
            sum = (uint16_t)(sum + v);
        }
        sum = (uint16_t)(sum + f[at + 3 * FRAME + col]);
        f[at + 3 * FRAME + col] = (uint8_t)(sum >> 3);
    }
}

static SC_M68K_HOT void Smooth7Weighted(uint8_t *f, unsigned col)
{
    size_t at = 0;
    for (;; at += FRAME)
    {
        uint16_t sum = 0;
        int k;
        for (k = 6; k >= 0; --k)
        {
            uint8_t v = f[at + k * FRAME + col];
            if (v == END)
            {
                return;
            }
            sum = (uint16_t)(sum + (v << 1));
        }
        sum = (uint16_t)(sum - f[at + col] - f[at + 6 * FRAME + col]);
        sum = (uint16_t)(sum + (f[at + 3 * FRAME + col] << 2));
        f[at + 3 * FRAME + col] = (uint8_t)(sum >> 4);
    }
}

static void Intrinsic(struct SCPhonemes *s, const struct SCVoiceData *v,
                      uint8_t *f)
{
    size_t at = 0;
    uint16_t i;
    for (i = 0;; ++i)
    {
        uint8_t p = s->phonemes[i], n = s->flags[i] & 0x3f;
        uint32_t a;
        unsigned k, count;
        if (p == END)
        {
            return;
        }
        count = n == 0 ? 0x10000UL : n;
        if (p == 0x42 || p == 0x45)
            for (k = 0; k < count; ++k, at += FRAME)
            {
                f[at + 7] += 10;
            }
        else if (p == 0x2f)
            for (k = 0; k < count; ++k, at += FRAME)
            {
                f[at + 7] = 0xe6;
            }
        else
        {
            a = InterpolateAttrs(v->attrs, p);
            if (a & ((1UL << 11) | (1UL << 16) | (1UL << 12)))
                for (k = 0; k < count; ++k, at += FRAME)
                {
                    f[at + 7] -= 6;
                }
            else if (a & 0x28001)
                for (k = 0; k < count; ++k, at += FRAME)
                {
                    f[at + 7] += (uint8_t)(((int8_t)(f[at] - 0x2b)) >> 2);
                }
            else
            {
                at += n * FRAME;
            }
        }
    }
}

static void Frication(struct SCPhonemes *s, const struct SCVoiceData *v,
                      uint8_t *f)
{
    size_t at = 0;
    uint16_t i = 0;
    int first = 1;
    for (;;)
    {
        uint8_t p = s->phonemes[i], n, half, quarter, next;
        uint32_t a;
        unsigned k, count;
        if (p == END)
        {
            return;
        }
        n = s->flags[i] & 0x3f;
        ++i;
        if (p == 1 || p == 2)
        {
            count = n == 0 ? 0x10000UL : n;
            for (k = 0; k < count; ++k, at += FRAME)
            {
                f[at] = f[at - FRAME];
                f[at + 1] = f[at - FRAME + 1];
                f[at + 2] = f[at - FRAME + 2];
            }
            first = 0;
            continue;
        }
        a = InterpolateAttrs(v->attrs, p);
        if ((a & (1UL << 9)) || !(a & 0x3000) || (int8_t)n <= 2)
        {
            at += n * FRAME;
            first = 0;
            continue;
        }
        if (!first && (InterpolateAttrs(v->attrs, s->phonemes[i - 2]) & (1UL << 2)))
        {
            f[at - FRAME + 3] = f[at - FRAME + 4] = f[at - FRAME + 5] = 0;
            f[at - 2 * FRAME + 3] = f[at - 2 * FRAME + 4] = f[at - 2 * FRAME + 5] = HOLE;
        }
        half = (f[at + 6] & 15) >> 1;
        quarter = (uint8_t)((half + 1) >> 1);
        f[at + 6] = (f[at + 6] & 0xf0) | quarter;
        f[at + FRAME + 6] = (f[at + FRAME + 6] & 0xf0) | half;
        at += n * FRAME;
        f[at - FRAME + 6] = (f[at - FRAME + 6] & 0xf0) | quarter;
        f[at - 2 * FRAME + 6] = (f[at - 2 * FRAME + 6] & 0xf0) | half;
        next = s->phonemes[i];
        if (next != END && (InterpolateAttrs(v->attrs, next) & (1UL << 2)))
        {
            f[at + 3] = f[at + 4] = f[at + 5] = 0;
            f[at + FRAME + 3] = f[at + FRAME + 4] = f[at + FRAME + 5] = HOLE;
            f[at + 2 * FRAME + 3] = f[at + 2 * FRAME + 4] = f[at + 2 * FRAME + 5] = HOLE;
        }
        first = 0;
    }
}

static void Nasalise(struct SCPhonemes *s, const struct SCVoiceData *v,
                     uint8_t *f)
{
    size_t at = 0;
    uint16_t i;
    const uint8_t *rows[6] = {v->f1, v->f2, v->f3, v->a1, v->a2, v->a3};
    for (i = 0;; ++i)
    {
        uint8_t p = s->phonemes[i], n = s->flags[i] & 0x3f;
        uint32_t a;
        unsigned k, b, count;
        if (p == END)
        {
            return;
        }
        a = InterpolateAttrs(v->attrs, p);
        if (a & (1UL << 16))
        {
            uint8_t col = p == 0x2a ? 0x60 : p == 0x2b ? 0x61 : p == 0x2c ? 0x62 : 0x63;
            count = n == 0 ? 0x10000UL : n;
            for (k = 0; k < count; ++k, at += FRAME) for (b = 0; b < 6; ++b)
                {
                    f[at + b] = rows[b][col];
                }
        }
        else if ((a & 1) && (InterpolateAttrs(v->attrs, s->phonemes[i + 1]) & (1UL << 16)))
        {
            uint8_t half = n >> 1, left;
            if (n < 3)
            {
                at += n * FRAME;
                continue;
            }
            at += half * FRAME;
            f[at] += 4;
            at += FRAME;
            left = (uint8_t)(n - half - 2);
            for (;;)
            {
                f[at] += 9;
                f[at + 3] -= f[at + 3] >> 2;
                at += FRAME;
                if (left-- == 0)
                {
                    break;
                }
            }
        }
        else
        {
            at += n * FRAME;
        }
    }
}

static void SmoothMouths(uint8_t *m, size_t total)
{
    uint16_t count = (uint16_t)((uint16_t)(total - 5) +1), k;
    unsigned j;
    uint8_t sum;
    for (k = 0; k < count; ++k)
    {
        sum = 0;
        for (j = 0; j < 3; ++j)
        {
            sum += (m[k + j] & 15);
        }
        sum += 2*(m[k+3]&15);
        for (j=4; j<7; ++j)
        {
            sum += (m[k + j] & 15);
        }
        m[k + 3] = (m[k + 3] & 0xf0) | (sum >> 3);
    }
    for (k = 0; k < count; ++k)
    {
        sum = 0;
        for (j = 0; j < 3; ++j)
        {
            sum += m[k+j]>>4;
        }
        sum += 2*(m[k+3]>>4);
        for (j=4; j<7; ++j)
        {
            sum += m[k + j] >> 4;
        }
        m[k + 3] = (m[k + 3] & 15) | ((sum << 1) & 0xf0);
    }
}

void SCFinishFrames(struct SCPhonemes *s, const struct SCVoiceData *v,
                      uint8_t *f, uint8_t *mouths, size_t total)
{
    unsigned c;
    size_t at;
    for (c = 0; c < 3; ++c)
    {
        FillZero(f, c);
    }
    FillZero(f, 7);
    Smooth7Plain(f, 1);
    Smooth7Weighted(f, 2);
    Intrinsic(s, v, f);
    Smooth7Plain(f, 7);
    Frication(s, v, f);
    for (c = 3; c < 6; ++c)
    {
        FillMarked(f, c);
    }
    for (at = 0; f[at + 3] != END; at += FRAME) if (f[at + 3] != HOLE)
        {
            f[at + 3] = v->gain[f[at + 3]];
            f[at + 4] = v->gain[f[at + 4]];
            f[at + 5] = v->gain[f[at + 5]];
        }
    Nasalise(s, v, f);
    if (mouths)
    {
        SmoothMouths(mouths, total);
    }
}
