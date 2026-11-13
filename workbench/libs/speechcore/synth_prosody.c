/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include "speechcore.h"
#include "synth_internal.h"

#include <string.h>

#define PA_DESCR 4
#define PA_VOICING 5
#define SY_PRIMARY 0x20
#define SY_LAST 0x40
#define SY_PAUSE 0x80

static uint32_t PhonemeAttrs(const uint32_t *attrs, uint8_t p)
{
    return p == 0xff ? 0 : attrs[p];
}

static int ScanPhrase(struct SCPhonemes *s, const uint32_t *attrs,
                      struct SCProsody *q)
{
    uint8_t *a4 = q->arr[PA_DESCR] + q->arr_at;
    uint16_t p = q->at_phoneme, st = q->at_stress, f = q->at_flag, n = 0;
    int first = -1;
    q->stresses = 0;
    for (;;)
    {
        uint8_t phoneme, mark, extra = 0;
        uint32_t a;
        unsigned level;

        if (p >= SC_ARRAY_SIZE || st >= SC_ARRAY_SIZE ||
                f >= SC_ARRAY_SIZE)
        {
            return -1;
        }
        phoneme = s->phonemes[p++];
        mark = s->stress[st++];
        ++f;
        if (phoneme == 0xff)
        {
            if (n == 0)
            {
                return 0;
            }
            a4[n - 1] |= SY_LAST;
            if (q->stresses == 0)
            {
                first = n;
            }
            q->first = (uint16_t)first;
            q->syllables = n;
            return n;
        }
        a = PhonemeAttrs(attrs, phoneme);
        if (a & (1UL << 26))
        {
            if (n == 0)
            {
                return -1;
            }
            ++q->boundaries;
            a4[n - 1] |= SY_PAUSE;
            if (a & (1UL << 19))
            {
                a4[n - 1] |= SY_LAST;
                if (q->stresses == 0)
                {
                    first = n;
                }
                q->first = (uint16_t)first;
                q->syllables = n;
                return n;
            }
            continue;
        }
        if (!(mark & 0x80))
        {
            continue;
        }
        level = mark;
        if (mark & 0x20)
        {
            level = mark & 15;
            while (level == 0)
            {
                if (p >= SC_ARRAY_SIZE || st >= SC_ARRAY_SIZE ||
                        f >= SC_ARRAY_SIZE)
                {
                    return -1;
                }
                ++f;
                ++p;
                mark = s->stress[st++];
                level = mark & 15;
            }
            level = ((level * 0xc7) & 0xffff) >> 7;
            if (s->stress[st - 1] & 0x40)
            {
                level += 2;
            }
            if ((int8_t)level > 4)
            {
                ++q->stresses;
                q->last = n;
                extra = SY_PRIMARY;
                if (first < 0)
                {
                    first = n;
                }
            }
        }
        if (q->arr_at + n >= 0x80)
        {
            return -1;
        }
        a4[n++] = (level & 15) | extra;
        if (++q->total == 0x80)
        {
            return -1;
        }
    }
}

static int MarkBoundaries(struct SCPhonemes *s, const uint32_t *attrs,
                          struct SCProsody *q)
{
    uint8_t *a3 = q->arr[3] + q->arr_at, *a4 = q->arr[4] + q->arr_at;
    uint16_t p = q->at_phoneme, st = q->at_stress, f = q->at_flag, n = 0;
    int i;
    for (;;)
    {
        uint8_t phoneme, mark, flag;

        if (p >= SC_ARRAY_SIZE || st >= SC_ARRAY_SIZE ||
                f >= SC_ARRAY_SIZE)
        {
            return -1;
        }
        phoneme = s->phonemes[p++];
        mark = s->stress[st++];
        flag = s->flags[f++];
        if (phoneme == 0xff)
        {
            --p;
            --st;
            --f;
            break;
        }
        if (phoneme == 4)
        {
            if (n == 0)
            {
                return -1;
            }
            a3[n - 1] |= 0x90;
        }
        else if (PhonemeAttrs(attrs, phoneme) & (1UL << 19))
        {
            break;
        }
        if (mark & 0x80)
        {
            if (q->arr_at + n >= 0x80)
            {
                return -1;
            }
            ++n;
        }
        if (flag & 0x20)
        {
            if (n == 0)
            {
                return -1;
            }
            a3[n - 1] = 2;
            continue;
        }
        if (flag & 0x10)
        {
            if (n == 0)
            {
                return -1;
            }
            int8_t v = (int8_t)((a3[n - 1] & 15) << 4);
            v >>= 4;
            if (v == 0 || v <= -2)
            {
                a3[n - 1] |= 0x0e;
            }
        }
    }
    q->at_phoneme = p;
    q->at_stress = st;
    q->at_flag = f;
    {
        uint8_t carry = 0;
        for (i = (int)n - 1; i >= 0; --i)
        {
            int at = i, step;
            a4[i] |= carry;
            if (a3[i] == 2)
            {
                step = 1;
                carry = 0;
            }
            else if (a3[i] == 0x0e)
            {
                step = -1;
                carry = 0x10;
                a4[i] |= carry;
            }
            else
            {
                continue;
            }
            for (;;)
            {
                if (a4[at]&SY_PRIMARY)
                {
                    a3[at] = a3[i];
                    if (at != i)
                    {
                        a3[i] = 0;
                    }
                    break;
                }
                at += step;
                if (at < 0 || at >= (int)n)
                {
                    a3[i] = 0;
                    break;
                }
            }
        }
    }
    return n;
}

static int NextPhrase(struct SCPhonemes *s, const uint32_t *attrs,
                      struct SCProsody *q)
{
    uint16_t n, i;
    int marked;
    uint8_t *a3, *a4, *a5, ended, code = 0;
    ++q->pass;
    {
        int found = ScanPhrase(s, attrs, q);
        if (found <= 0)
        {
            return found;
        }
    }
    marked = MarkBoundaries(s, attrs, q);
    if (marked <= 0 || q->arr_at + marked > 0x80)
    {
        return -1;
    }
    n = (uint16_t)marked;
    a3 = q->arr[3] + q->arr_at;
    a4 = q->arr[4] + q->arr_at;
    a5 = q->arr[5] + q->arr_at;
    for (i = n; i > 0; --i)
    {
        a5[i - 1] |= 1;
    }
    if (q->at_phoneme == 0 || q->at_phoneme > SC_ARRAY_SIZE)
    {
        return -1;
    }
    ended = s->phonemes[q->at_phoneme - 1];
    if (ended == 1)
    {
        code = 4;
    }
    if (ended == 2)
    {
        code = 8;
    }
    for (i = n; i > 0; --i)
    {
        a5[i - 1] |= code;
        if (a4[i - 1]&SY_PAUSE)
        {
            break;
        }
    }
    if (ended != 1 && ended != 2)
    {
        a3[n - 1] |= a5[n - 1] == 8 ? 0x30 : 0xb0;
        if (q->stresses)
        {
            i = n - 1;
            while (i > 0 && !(a4[i]&SY_PRIMARY))
            {
                --i;
            }
            a3[i] = (a3[i] & 0xf0) | 4;
        }
    }
    return 1;
}

static int16_t SignedWord(int32_t v)
{
    return (int16_t)v;
}
static int8_t SignedByte(int32_t v)
{
    return (int8_t)v;
}
static int32_t MultiplySignedWords(int32_t a, int32_t b)
{
    return (int32_t)SignedWord(a) * SignedWord(b);
}
static int16_t Round7(int32_t v)
{
    return (int16_t)(SignedWord(SignedWord(v) + (v < 0 ? -0x40 : 0x40)) >> 7);
}

static uint16_t PhrasePitch(struct SCProsody *q)
{
    uint8_t *a1 = q->arr[1] + q->arr_at;
    uint16_t spread = (uint16_t)((q->boundaries + 4) / 3);
    uint16_t rise = (uint16_t)((uint16_t)(6 - q->pass) << 1), peak;
    if (rise & 0x8000)
    {
        rise = 0;
    }
    peak = (uint16_t)(((rise * spread) & 0xffff) +0x7b-(q->pass << 3));
    if (SignedWord(peak) <= 0x7d)
    {
        peak = 0x7d;
    }
    if (SignedWord(peak) >= 0xa5)
    {
        peak = 0xa5;
    }
    a1[q->first] = (uint8_t)peak;
    return peak;
}

static void SyllablePitch(struct SCProsody *q, uint16_t pitch)
{
    uint8_t *a1 = q->arr[1] + q->arr_at, *a4 = q->arr[4] + q->arr_at;
    uint8_t *a5 = q->arr[5] + q->arr_at;
    uint16_t floor = (a5[q->syllables - 1] & 12) == 8 ? 0x73 : 0x6e;
    uint16_t drop = (uint8_t)(pitch - floor) / q->stresses, big = drop, small = drop;
    uint16_t i, after;
    uint8_t step, level;
    if (SignedWord(q->stresses) >= 4)
    {
        uint16_t extra = (uint16_t)(((drop * 0x13) & 0xffff) >> 7);
        big = drop + extra;
        small = (uint16_t)(drop - (((extra << 1) & 0xffff) / (q->stresses - 3)));
    }
    step = (uint8_t)big;
    level = a1[q->first];
    i = q->first + 1;
    for (;;)
    {
        if (a4[i]&SY_PRIMARY)
        {
            if (i == q->last)
            {
                step = (uint8_t)big;
            }
            level -= step;
            a1[i] = level;
            step = (uint8_t)small;
        }
        ++i;
        if (SignedWord(q->syllables) <= SignedWord(i))
        {
            break;
        }
    }
    /* The neighbour pass is observably inert in 33.2: markVoiced makes the
       low two VOICING bits exactly one for every syllable. */
    after = q->last;
    (void)after;
    for (i = q->last;; --i)
    {
        if (a4[i]&SY_PRIMARY)
        {
            int16_t weight = SignedWord((a4[i] & 15) -8);
            uint16_t above = (uint16_t)((((a1[i] - 0x6e) & 0xffff) * 0x0d) & 0xffff) >> 7;
            a1[i] = (uint8_t)(a1[i] + weight * SignedWord(above));
        }
        if (i == 0)
        {
            break;
        }
    }
}

static void SyllableRange(struct SCProsody *q)
{
    uint8_t *a1 = q->arr[1] + q->arr_at, *a3 = q->arr[3] + q->arr_at;
    uint8_t *a4 = q->arr[4] + q->arr_at, *a6 = q->arr[6] + q->arr_at, *a7 = q->arr[7] + q->arr_at;
    uint16_t i;
    for (i = q->last;; --i)
    {
        if (a4[i]&SY_PRIMARY)
        {
            int16_t above = SignedWord(a1[i] - 0x6e);
            uint8_t nib = a3[i] & 15;
            int16_t cad = SignedWord(nib | ((nib & 8) ? 0xfff0 : 0));
            int32_t climb = MultiplySignedWords(cad, 0x1a) +0x80;
            climb = MultiplySignedWords(climb, above) >> 7;
            a6[i] = (uint8_t)(MultiplySignedWords(climb, 0x33) >> 7);
            {
                uint8_t d = (uint8_t)(-(MultiplySignedWords(MultiplySignedWords(cad - 1, above), 0x1a) >> 7));
                a7[i] = (d & 0x80) ? 0 : d;
            }
        }
        if (i == 0)
        {
            break;
        }
    }
    a6[q->first] = (uint8_t)(a1[q->first] - 0x6e);
}

static void LinkSyllables(struct SCProsody *q)
{
    uint8_t *a1 = q->arr[1] + q->arr_at, *a3 = q->arr[3] + q->arr_at;
    uint8_t *a4 = q->arr[4] + q->arr_at, *a5 = q->arr[5] + q->arr_at;
    uint8_t *a6 = q->arr[6] + q->arr_at, *a7 = q->arr[7] + q->arr_at;
    int i;
    uint16_t next = q->last;
    for (i = (int)q->last - 1; i >= 0; --i)
    {
        int16_t gap;
        int widen;
        if (!(a4[i]&SY_PRIMARY))
        {
            continue;
        }
        gap = SignedWord(next - i - 2);
        if (gap < 0)
        {
            uint8_t d;
            a6[i] += Round7(MultiplySignedWords(SignedByte(a6[i]), -0x33));
            a6[next] += Round7(MultiplySignedWords(SignedByte(a6[next]), -0x33));
            a1[i] += Round7(MultiplySignedWords(a1[i] - 0x6e, -0x1a));
            a1[next] += Round7(MultiplySignedWords(a1[next] - 0x6e, 0x1a));
            d = (uint8_t)(a1[i] - a7[i] - a1[next] + a6[next]);
            if (SignedByte(d) < 0)
            {
                a6[next] -= d;
            }
            else
            {
                a7[i] += d;
            }
        }
        else if (gap > 0)
        {
            widen = gap == 1 ? 0x13 : gap == 2 ? 0x20 : 0x26;
            a6[i] += Round7(MultiplySignedWords(SignedByte(a6[i]), widen));
            a6[next] += Round7(MultiplySignedWords(SignedByte(a6[next]), widen));
            a1[next] += Round7(MultiplySignedWords(a1[next] - 0x6e, gap == 1 ? -0x13 : -0x20));
            a1[i] += Round7(MultiplySignedWords(a1[i] - 0x6e, gap == 1 ? 0x0d : 0x13));
            if (!((a4[next] & 0x10) && ((a3[next] & 15) == 0 || (a3[next] & 8))) && gap >= 2)
            {
                a6[next] = a1[next] - 0x69;
            }
        }
        next = (uint16_t)i;
    }
    for (i = q->last; i >= 0; --i)
    {
        uint8_t d = a4[i];
        if ((d & SY_PRIMARY) && (d & SY_PAUSE))
        {
            uint8_t punct = a5[i] & 12;
            if (punct == 8)
            {
                int16_t d1 = Round7(MultiplySignedWords(SignedByte(a7[i]), 0x66));
                int16_t d7 = a1[i];
                int k;
                a6[i] += d1;
                for (k = i - 1; k >= 0; --k)
                {
                    d1 = (d1 & 0xff00) | a1[k];
                    if (SignedWord(d7) <= SignedWord(d1))
                    {
                        d7 = (d7 & 0xff00) | (d1 & 255);
                    }
                }
                a7[i] = (uint8_t)(a1[i] - ((((uint16_t)d7 * 0x9a) & 0xffff) >> 7));
            }
            else if (punct == 4)
            {
                a7[i] = (uint8_t)(a1[i] - 0x4b);
            }
        }
    }
}

static void BoundaryFall(struct SCProsody *q)
{
    uint8_t *a3 = q->arr[3] + q->arr_at, *a7 = q->arr[7] + q->arr_at;
    int i;
    for (i = q->syllables - 1; i >= 0; --i)
    {
        uint8_t m = a3[i] & 0xf0;
        if (m)
        {
            a7[i] += Round7(MultiplySignedWords(SignedByte(a7[i]), (m & 0x80) ? 0x26 : -0x66));
        }
    }
}

static const uint8_t glide_factors[6] = {0x3a, 0x64, 0x49, 0x4d, 0x56, 0x80};
static uint16_t Glide(uint8_t *a0, uint8_t *a1, uint8_t *a2, uint16_t at,
                      uint16_t step, unsigned from, unsigned to)
{
    unsigned j;
    for (j = from; j < to; ++j)
    {
        a0[at + 1] = a2[at];
        step = (uint16_t)(((step * glide_factors[j]) & 0xffff) >> 7);
        ++at;
        a2[at] = a0[at] - step;
        a1[at] = a2[at] + 5;
    }
    return at;
}

static void FillContours(struct SCProsody *q)
{
    uint8_t *a0 = q->arr[0] + q->arr_at, *a1 = q->arr[1] + q->arr_at, *a2 = q->arr[2] + q->arr_at;
    uint8_t *a4 = q->arr[4] + q->arr_at, *a5 = q->arr[5] + q->arr_at, *a6 = q->arr[6] + q->arr_at, *a7 = q->arr[7] + q->arr_at;
    int i;
    uint16_t scan = q->first, remaining = q->stresses;
    for (i = q->last; i >= 0; --i)if (a4[i]&SY_PRIMARY)
        {
            a0[i] = a1[i] - a6[i];
            a2[i] = a1[i] - a7[i];
        }
    for (;;)
    {
        uint16_t at = scan, step = 0;
        int16_t between;
        if (SignedWord(--remaining) <= 0)
        {
            break;
        }
        do
        {
            ++scan;
        }
        while (!(a4[scan]&SY_PRIMARY));
        between = SignedWord(scan - at - 1);
        if (!between)
        {
            continue;
        }
        step = (uint8_t)(a2[at] - a0[scan]);
        if (SignedByte(step) < 0)
        {
            uint8_t half = (uint8_t)(-step) >> 1;
            a0[scan] -= half;
            a6[scan] += half;
            a2[at] += half;
            a7[at] -= half;
            step = 0;
        }
        if (between >= 3)
        {
            step = (uint8_t)(a2[at] - 0x69);
        }
        if (between == 1)
        {
            at = Glide(a0, a1, a2, at, step, 5, 6);
        }
        else if (between == 2)
        {
            at = Glide(a0, a1, a2, at, step, 3, 5);
        }
        else
        {
            at = Glide(a0, a1, a2, at, step, 0, 3);
        }
        for (i = 0; i < between - 3; ++i)
        {
            uint8_t held = a2[at];
            ++at;
            a0[at] = a2[at] = held;
            a1[at] = held + ((a4[at] & 15) << 1);
        }
    }
    for (i = (int)q->first - 1; i >= 0; --i)
    {
        a1[i] = 0x6e + ((a4[i] & 15) << 1);
        a0[i] = a2[i] = 0x6e;
    }
    {
        uint16_t last = q->syllables - 1, at = 0, count;
        uint8_t punct;
        if (a4[last]&SY_PRIMARY)
        {
            return;
        }
        punct = a5[last] & 12;
        for (i = last - 1; i >= 0; --i)if (a4[i]&SY_PRIMARY)
            {
                at = (uint16_t)i;
                break;
            }
        count = (uint16_t)(last - at);
        if (count)
        {
            uint16_t step = (uint16_t)(a2[at] - 0x6e);
            if (punct == 4)
            {
                step += 35;
            }
            step /= count;
            for (i = 0; i < count; ++i)
            {
                a0[at + 1] = a2[at];
                ++at;
                a2[at] = (uint8_t)(a0[at] - step);
                a1[at] = a2[at] + 5;
            }
        }
        if (punct == 8)
        {
            uint16_t high = 0;
            for (i = q->syllables - 1; i >= 0; --i)if (SignedWord(high) <= a1[i])
                {
                    high = a1[i];
                }
            high = (uint16_t)(((high * 0x9a) & 0xffff) >> 7);
            a2[last] = (uint8_t)high;
            a1[last] = (uint8_t)(high + 5);
            a7[last] = 0;
            if (q->syllables > 1)
            {
                a0[last] = a2[last - 1];
            }
            return;
        }
        a2[at] = punct == 4 ? 0x4b : 0x6e;
    }
}

static int Coarticulate(struct SCPhonemes *s, const uint32_t *attrs,
                        struct SCProsody *q)
{
    uint8_t *a0 = q->arr[0] + q->arr_at, *a1 = q->arr[1] + q->arr_at, *a2 = q->arr[2] + q->arr_at;
    uint8_t *a4 = q->arr[4] + q->arr_at, *a6 = q->arr[6] + q->arr_at, *a7 = q->arr[7] + q->arr_at;
    int p = q->at_phoneme - 1, st = q->at_stress - 1, i;
    for (i = q->syllables - 1; i >= 0; --i)
    {
        uint32_t onset;
        int by, d, k;
        do
        {
            if (p <= 0 || st <= 0)
            {
                return 0;
            }
            --p;
            --st;
        }
        while (!(s->stress[st] & 0x80));
        if (!(a4[i]&SY_PRIMARY))
        {
            continue;
        }
        onset = PhonemeAttrs(attrs, s->phonemes[p]);
        if (onset & 2)
        {
            by = 0x1a;
            if (!(onset & (1UL << 9)))
            {
                int lift = Round7(MultiplySignedWords(a1[i] - 0x6e, 0x1a));
                a1[i] += lift;
                a7[i] += lift;
                a6[i] += lift;
                by = 0x66;
            }
            d = Round7(MultiplySignedWords(SignedByte(a6[i]), by));
            a0[i] += d;
            a6[i] -= d;
        }
        if ((a4[i]&SY_PAUSE) ||
                (i + 1 < (int)q->syllables &&
                 (a4[i + 1]&SY_PRIMARY)))
        {
            if (i == (int)q->syllables - 1)
            {
                continue;
            }
            by = 0x40;
        }
        else
        {
            k = 0;
            while (st + k < SC_ARRAY_SIZE &&
                    (s->stress[st + k] & 15) == 0)
            {
                ++k;
            }
            if (st + k >= SC_ARRAY_SIZE)
            {
                return 0;
            }
            ++k;
            for (;;)
            {
                if (p + k >= SC_ARRAY_SIZE)
                {
                    return 0;
                }
                uint8_t next = s->phonemes[p + k];
                uint32_t a = PhonemeAttrs(attrs, next);
                if (next == 0x2f || (a & 1))
                {
                    by = 0x1a;
                    break;
                }
                if (!(a & (1UL << 9)))
                {
                    by = 0x56;
                    break;
                }
                ++k;
            }
        }
        d = Round7(MultiplySignedWords(SignedByte(a7[i]), by));
        a2[i] += d;
        a7[i] -= d;
    }
    return 1;
}

static int PitchBody(struct SCPhonemes *s, const uint32_t *attrs,
                     struct SCProsody *q, uint8_t mode)
{
    if (mode == 0)
    {
        if (q->stresses)
        {
            SyllablePitch(q, PhrasePitch(q));
            SyllableRange(q);
            LinkSyllables(q);
        }
        BoundaryFall(q);
        FillContours(q);
        if (!Coarticulate(s, attrs, q))
        {
            return 0;
        }
    }
    q->arr_at += q->syllables;
    return 1;
}

int SCBuildProsody(struct SCPhonemes *s, const uint32_t *attrs,
                     struct SCProsody *q, uint8_t mode)
{
    int rc;
    (void)mode;
    memset(q, 0, sizeof(*q));
    q->at_phoneme = q->at_stress = q->at_flag = 2;
    while ((rc = NextPhrase(s, attrs, q)) > 0)
    {
        if (!PitchBody(s, attrs, q, mode))
        {
            return SC_ERR_BAD_PHONEME;
        }
    }
    return rc < 0 ? SC_ERR_BAD_PHONEME : SC_OK;
}
