/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include "synth_internal.h"

#include <string.h>

#define FRAME 8
#define DUR 0x3f

static uint32_t FrameAttrs(const uint32_t *attrs, uint8_t p)
{
    return p == 0xff ? 0 : attrs[p];
}

static int Compact(struct SCPhonemes *s, const uint32_t *attrs)
{
    uint16_t r = 0, w = 0;
    for (; r < SC_ARRAY_SIZE; ++r)
    {
        uint8_t p = s->phonemes[r];
        if (p == 0xff)
        {
            s->phonemes[w] = s->stress[w] = s->flags[w] = 0xff;
            return 1;
        }
        if (FrameAttrs(attrs, p) & (1UL << 20))
        {
            continue;
        }
        if (w >= SC_ARRAY_SIZE - 1)
        {
            return 0;
        }
        s->phonemes[w] = p;
        s->stress[w] = s->stress[r];
        s->flags[w++] = s->flags[r];
    }
    return 0;
}

static int Continuation(struct SCPhonemes *s,
                        const struct SCVoiceData *v)
{
    uint16_t i = 0;
    for (; i < SC_ARRAY_SIZE;)
    {
        uint8_t p = s->phonemes[i], half;
        uint32_t a;
        if (p == 0xff)
        {
            return 1;
        }
        if (p == 0x17)
        {
            if (i == 0)
            {
                return 0;
            }
            unsigned sum = (s->flags[i] & DUR) + (s->flags[i - 1] & DUR);
            sum = (sum - (sum >> 2)) >> 1;
            s->flags[i] = (s->flags[i] & 0xc0) | sum;
            s->flags[i - 1] = (s->flags[i - 1] & 0xc0) | sum;
            ++i;
            continue;
        }
        a = FrameAttrs(v->attrs, p);
        if (a & (1UL << 7))
        {
            if (i + 1 >= SC_ARRAY_SIZE)
            {
                return 0;
            }
            half = (s->flags[i] & DUR) >> 1;
            s->flags[i + 1] |= half;
            s->flags[i] = (s->flags[i] & 0xc0) | (uint8_t)(half + 1);
            i += 2;
            continue;
        }
        if (!(a & (1UL << 21)))
        {
            ++i;
            continue;
        }
        if (i == 0)
        {
            return 0;
        }
        s->stress[i] = s->stress[i - 1] & 0x7f;
        s->flags[i] = (s->stress[i] & 0x10) ? v->stressed[p] : v->unstressed[p];
        ++i;
        if (a & (1UL << 13))
        {
            if (i >= SC_ARRAY_SIZE)
            {
                return 0;
            }
            uint32_t next = FrameAttrs(v->attrs, s->phonemes[i]);
            uint8_t code = 4;
            if (next & (1UL << 3))
            {
                code = 3;
            }
            else if (next & (1UL << 6))
            {
                code = 6;
            }
            else if (next & (1UL << 5))
            {
                code = 5;
            }
            s->stress[i - 1] = (s->stress[i - 1] & 0xf0) | code;
        }
    }
    return 0;
}

size_t SCFrameStorageSize(struct SCPhonemes *s,
                             const struct SCVoiceData *v,
                             int mouths, size_t *total_out)
{
    size_t total = 0;
    uint16_t i;
    if (!Compact(s, v->attrs) || !Continuation(s, v))
    {
        return 0;
    }
    for (i = 0; i < SC_ARRAY_SIZE && s->phonemes[i] != SC_TERMINATOR; ++i)
    {
        total += s->flags[i] & DUR;
    }
    if (i == SC_ARRAY_SIZE)
    {
        return 0;
    }
    *total_out = total;
    if (total > (SIZE_MAX - 8) / 8)
    {
        return 0;
    }
    /* The original mouth smoother reads six clear bytes beyond the logical
       stream. Reserve them explicitly instead of relying on allocator dirt. */
    return (total + 1) * 8 + (mouths ? total + 6 : 0);
}

static void Blend(struct SCPhonemes *s, const struct SCVoiceData *v,
                  uint8_t *f)
{
    size_t at = 0;
    uint16_t i;
    for (i = 0;; ++i)
    {
        uint8_t here, next, mine, theirs, w;
        unsigned k;
        at += (s->flags[i] & DUR) * FRAME;
        next = s->phonemes[i + 1];
        if (next == 0xff)
        {
            return;
        }
        if ((FrameAttrs(v->attrs, next) & (1UL << 21)) &&
                (FrameAttrs(v->attrs, next) & 0x2c00))
        {
            continue;
        }
        if (at < FRAME)
        {
            continue;
        }
        here = s->phonemes[i];
        mine = v->rank[here];
        theirs = v->rank[next];
        w = v->weight[theirs >= mine ? next : here];
        for (k = 0; k < 6; ++k)
        {
            uint8_t from = f[at + k], to = f[at - FRAME + k];
            int base = theirs >= mine ? to : from;
            int other = theirs >= mine ? from : to;
            int d;
            if (k < 3 && (base == 0 || other == 0))
            {
                continue;
            }
            d = (int16_t)(other - base);
            f[at + k] = (uint8_t)(((d * w) >> 5) + base);
        }
    }
}

static int MarkOne(uint8_t *f, size_t bytes, size_t at, int keep)
{
    if (at > bytes || bytes - at < 6)
    {
        return 0;
    }
    f[at] = f[at + 1] = f[at + 2] = 0;
    if (!keep)
    {
        f[at + 3] = f[at + 4] = f[at + 5] = 0xfe;
    }
    return 1;
}

static int MarkTransitions(struct SCPhonemes *s,
                           const struct SCVoiceData *v, uint8_t *f,
                           size_t bytes)
{
    size_t at = (s->flags[0] & DUR) * FRAME;
    uint16_t i;
    for (i = 1;; ++i)
    {
        uint8_t here = s->phonemes[i], prev, next, duration, mine;
        uint8_t head, tail, middle = 0, shrink = 2;
        uint32_t a;
        int keep_head = 0, keep_tail = 0;
        unsigned k;
        if (here == 0xff)
        {
            return 1;
        }
        prev = s->phonemes[i - 1];
        next = s->phonemes[i + 1];
        duration = s->flags[i] & DUR;
        a = FrameAttrs(v->attrs, here);
        if (a & ((1UL << 10) | (1UL << 11)))
        {
            at += duration * FRAME;
            continue;
        }
        mine = v->rank[here];
        head = v->rank[prev] > mine ? v->transition_in[prev] :
               v->transition_out[here];
        tail = next == 0xff ? 0 :
               v->rank[next] >= mine ? v->transition_in[next] :
               v->transition_out[here];
        if (a & (1UL << 2))
        {
            uint32_t pa = FrameAttrs(v->attrs, prev);
            int check = 0;
            if (pa & ((1UL << 10) | (1UL << 11)))
            {
                keep_head = 1;
                check = 1;
            }
            else if ((pa & (1UL << 12)) && !(pa & (1UL << 9)))
            {
                check = 1;
            }
            if (check && (a & ((1UL << 15) | (1UL << 17))))
            {
                head = 2;
            }
            if (FrameAttrs(v->attrs, next) & ((1UL << 10) | (1UL << 11)))
            {
                keep_tail = 1;
            }
        }
        {
            uint16_t span = (uint16_t)(duration - 1);
            head = (uint8_t)(head - 1);
            if (head > 0x7f)
            {
                head = 0;
            }
            for (;;)
            {
                if ((int8_t)(head + tail) < (int8_t)span)
                {
                    middle = (uint8_t)(span - head - tail);
                    break;
                }
                head = (uint8_t)(head - 1);
                if (head > 0x7f)
                {
                    head = (uint8_t)span;
                    tail = middle = 0;
                    break;
                }
                tail = (uint8_t)(tail - 1);
                if (tail > 0x7f)
                {
                    head = (uint8_t)span;
                    tail = middle = 0;
                    break;
                }
                if (--shrink == 0)
                {
                    head = (uint8_t)span;
                    tail = middle = 0;
                    break;
                }
            }
        }
        at += FRAME;
        for (k = 0; k < head; ++k, at += FRAME)
        {
            if (!MarkOne(f, bytes, at, keep_head))
            {
                return 0;
            }
        }
        at += middle * FRAME;
        for (k = 0; k < tail; ++k, at += FRAME)
        {
            if (!MarkOne(f, bytes, at, keep_tail))
            {
                return 0;
            }
        }
    }
}

int SCBuildFrames(struct SCPhonemes *s, const struct SCVoiceData *v,
                  int alternate, uint8_t *f, uint8_t *mouths,
                  size_t total)
{
    size_t at = 0, mouth_at = 0;
    uint16_t i;
    const uint8_t *f1 = alternate ? v->alt_f1 : v->f1;
    const uint8_t *f2 = alternate ? v->alt_f2 : v->f2;
    const uint8_t *f3 = alternate ? v->alt_f3 : v->f3;
    for (i = 0;; ++i)
    {
        uint8_t p = s->phonemes[i], duration, st, a1, a2, a3, voicing, src;
        uint32_t a;
        unsigned k, n;
        if (p == 0xff)
        {
            memset(f + at, 0xff, FRAME);
            break;
        }
        duration = s->flags[i] & DUR;
        a = FrameAttrs(v->attrs, p);
        st = s->stress[i];
        a1 = v->a1[p];
        a2 = v->a2[p];
        a3 = v->a3[p];
        if (st & 0x10)
        {
            if (a1)
            {
                a1 = a1 + 2 > 0x1f ? 0x1f : a1 + 2;
            }
            if (a2)
            {
                a2 = (uint8_t)(a2 + 2);
            }
            if (a3)
            {
                a3 = (uint8_t)(a3 + 2);
            }
        }
        voicing = v->voicing[p];
        if ((a & (1UL << 21)) && (a & (1UL << 13)))
        {
            voicing |= (st & 7) << 4;
        }
        src = (i != 0 && (p == 1 || p == 2)) ? s->phonemes[i - 1] : p;
        if (voicing && !(st & 0x10)) voicing = (voicing & 0xf0) | ((voicing & 15)
                                                   >> 1);
        n = duration;
        for (k = 0; k < n; ++k)
        {
            if (mouths)
            {
                mouths[mouth_at++] = v->mouth[p];
            }
            f[at] = f1[src];
            f[at + 1] = f2[src];
            f[at + 2] = f3[src];
            f[at + 3] = a1;
            f[at + 4] = a2;
            f[at + 5] = a3;
            f[at + 6] = voicing;
            at += FRAME;
        }
    }
    for (at = 0; f[at] != 0xff; at += FRAME)
    {
        f[at + 7] = 0;
    }
    f[7] = 0xa0;
    Blend(s, v, f);
    return MarkTransitions(s, v, f, total * FRAME);
}
