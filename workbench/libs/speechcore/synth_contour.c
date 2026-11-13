/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include "synth_internal.h"

#define FRAME 8

static uint32_t ContourAttrs(const uint32_t *attrs, uint8_t p)
{
    return p == 0xff ? 0 : attrs[p];
}

static void MarkContour(struct SCPhonemes *s, const uint32_t *attrs)
{
    int i;
    for (i = 0; i < (int)s->count && s->stress[i] != 0xff; ++i)
    {
        s->stress[i] &= 0xf0;
    }
    i = -1;
    for (;;)
    {
        uint32_t a;
        uint8_t st;
        ++i;
        if (i >= (int)s->count || s->phonemes[i] == 0xff)
        {
            return;
        }
        a = ContourAttrs(attrs, s->phonemes[i]);
        st = s->stress[i];
        while (st & 0x80)
        {
            while (!(a & 1))
            {
                ++i;
                if (i >= (int)s->count || s->phonemes[i] == 0xff)
                {
                    return;
                }
                a = ContourAttrs(attrs, s->phonemes[i]);
                st = s->stress[i];
                if ((st & 0x80) || (a & (1UL << 25)))
                {
                    s->stress[i - 1] |= 1;
                    s->stress[i] |= 6;
                    goto marked_again;
                }
            }
            s->stress[i] |= 1;
            if ((a & (1UL << 7)) || (i + 1 < (int)s->count &&
                                     (s->phonemes[i + 1] == 0x18 || s->phonemes[i + 1] == 0x17)))
            {
                if (i + 2 < (int)s->count)
                {
                    s->stress[i + 2] |= 2;
                }
            }
            else if (i + 1 < (int)s->count)
            {
                s->stress[i + 1] |= 2;
            }
            {
                int end = i + 1;
                for (;;)
                {
                    ++i;
                    if (i >= (int)s->count || s->phonemes[i] == 0xff)
                    {
                        i = (int)s->count;
                        break;
                    }
                    a = ContourAttrs(attrs, s->phonemes[i]);
                    st = s->stress[i];
                    if ((st & 0x80) || (a & (1UL << 25)))
                    {
                        break;
                    }
                    if (a & (1UL << 9))
                    {
                        end=i+1;
                    }
                }
                if (end<(int)s->count)
                {
                    s->stress[end] |= 4;
                }
            }
marked_again:
            ;
        }
    }
}

static uint8_t PeriodFor(uint16_t c, uint8_t v)
{
    return v == 0 ? 0xff : (uint8_t)(c / v);
}

static int PutPitch(uint8_t *f, size_t bytes, size_t at, uint8_t value)
{
    if (at >= bytes)
    {
        return 0;
    }
    f[at] = value;
    return 1;
}

static void AssignPitch(struct SCPhonemes *s, struct SCProsody *q,
                        uint8_t *f, size_t bytes, uint16_t pitch, uint8_t mode)
{
    uint16_t c = (uint16_t)(0x12a188UL / pitch), i = 0, v = 0;
    size_t at = 0;
    if (mode == 1)
    {
        uint8_t flat = PeriodFor(c, 0x6e);
        for (at = 7; at < bytes && f[at] != 0xff; at += 8)
        {
            f[at] = flat;
        }
        return;
    }
    for (;;)
    {
        uint8_t duration, hi, mid, lo, rise;
        uint16_t span, off, travel, scaled;
        if (i >= s->count || s->flags[i] == 0xff)
        {
            if (v == 0)
            {
                uint8_t flat = PeriodFor(c, 0x6e);
                for (at = 7; at + 8 < bytes; at += 8)
                {
                    f[at] = flat;
                }
            }
            else if (at > 0)
            {
                PutPitch(f, bytes, at - 1, PeriodFor(c, q->arr[2][v - 1]));
            }
            return;
        }
        duration = s->flags[i] & 0x3f;
        if (!(s->stress[i++] & 1))
        {
            at += duration * FRAME;
            continue;
        }
        hi = q->arr[0][v];
        mid = q->arr[1][v];
        lo = q->arr[2][v];
        rise = (q->arr[3][v] & 0x70) >> 1;
        ++v;
        PutPitch(f, bytes, at + 7, PeriodFor(c, hi));
        span = duration * FRAME;
        for (;;)
        {
            uint8_t d;
            if (i >= s->count || s->flags[i] == 0xff)
            {
                break;
            }
            d = s->flags[i] & 0x3f;
            {
                int fall = s->stress[i] & 2;
                ++i;
                if (fall)
                {
                    break;
                }
            }
            span = (uint16_t)(span + d * FRAME);
        }
        off = span;
        if (rise)
        {
            uint16_t frames = off >> 3;
            off = (uint16_t)((frames - (frames >> 1)) << 3);
        }
        if (off == 0)
        {
            off = FRAME;
        }
        PutPitch(f, bytes, at + off - 1, PeriodFor(c, lo));
        travel = (uint16_t)(hi > mid ? hi - mid : mid - hi);
        scaled = (uint16_t)(travel << 5);
        travel = (uint16_t)(travel + (mid > lo ? mid - lo : lo - mid));
        if (travel)
        {
            uint16_t m = (uint16_t)(scaled / travel);
            m = (uint16_t)(m * off);
            m = (uint16_t)((m >> 8) << 3);
            if (m == 0)
            {
                m = FRAME;
            }
            PutPitch(f, bytes, at + m - 1, PeriodFor(c, mid));
        }
        --i;
        at += span;
        if (!rise)
        {
            continue;
        }
        for (;;)
        {
            uint8_t d;
            if (i >= s->count || s->flags[i] == 0xff)
            {
                break;
            }
            d = s->flags[i] & 0x3f;
            {
                int end = s->stress[i] & 4;
                ++i;
                if (end)
                {
                    break;
                }
            }
            at += d * FRAME;
        }
        if (at > 0)
        {
            PutPitch(f, bytes, at - 1, PeriodFor(c, (uint8_t)(lo + rise)));
        }
        if (i > 0)
        {
            --i;
        }
    }
}

int SCApplyContourWithAttrs(struct SCPhonemes *s, const uint32_t *attrs,
                                struct SCProsody *q, uint8_t *frames,
                                size_t frame_bytes, uint16_t pitch, uint8_t mode)
{
    MarkContour(s, attrs);
    AssignPitch(s, q, frames, frame_bytes, pitch, mode);
    return 0;
}
