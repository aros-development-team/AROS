/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include "speechcore.h"
#include "synth_internal.h"

#include <string.h>

#define ATTR_STRESSABLE (1UL << 0)
#define ATTR_CONSONANT (1UL << 1)
#define ATTR_VOICELESS_STOP (1UL << 11)
#define ATTR_TERMINAL (1UL << 25)
#define ATTR_PAUSE (1UL << 26)
#define ATTR_ILLEGAL (1UL << 27)

static uint32_t AttrOf(const uint32_t *attrs, uint8_t p)
{
    return p == SC_TERMINATOR ? 0 : attrs[p];
}

static uint16_t NameWord(const struct SCVoiceData *voice, uint16_t i)
{
    return ((uint16_t)(uint8_t)voice->names[i][0] << 8) |
           (uint8_t)voice->names[i][1];
}

static int FindWord(const struct SCVoiceData *voice, uint16_t word)
{
    uint16_t i;
    for (i = 0; i < voice->count; ++i)
        if (NameWord(voice, i) == word)
        {
            return i;
        }
    return -1;
}

int SCParsePhonemes(struct SCPhonemes *s, const struct SCVoiceData *voice,
                      const uint8_t *input, size_t length,
                      size_t *error_offset)
{
    size_t at = 0;
    uint16_t n = 4;
    int dash = 1;

    memset(s, 0, sizeof(*s));
    s->phonemes[2] = 0x15;
    if (length == 0)
    {
        return SC_OK;
    }

    for (;;)
    {
        uint8_t c0 = at < length ? input[at] : 0;
        uint8_t c1 = at + 1 < length ? input[at + 1] : 0;
        uint16_t word;
        unsigned taken;
        int idx;
        uint32_t a;

        if (c0 == 0 || c0 == '#')
        {
            break;
        }
        word = ((uint16_t)c0 << 8) | c1;
        taken = 2;
        idx = FindWord(voice, word);
        if (idx < 0)
        {
            word &= 0xff00;
            taken = 1;
            idx = FindWord(voice, word);
            if (idx < 0)
            {
                goto bad_here;
            }
        }
        if (word >= 0x3000 && word <= 0x3900)
        {
            at += taken;
            if (word != 0x3000)
            {
                if (!(AttrOf(voice->attrs, s->phonemes[n - 1]) &
                        ATTR_STRESSABLE))
                {
                    at -= taken;
                    goto bad_here;
                }
                s->stress[n - 1] = 0x30 | ((word >> 8) & 15);
            }
            if (at >= length)
            {
                break;
            }
            s->stress[n] = s->flags[n] = 0;
            continue;
        }

        at += taken;
        a = AttrOf(voice->attrs, (uint8_t)idx);
        /* These zero-duration internal allophones make the original 68k
           writer run 65,536 frames past its allocation.  Preserve the
           public device's safe failure contract instead of its corruption. */
        if (idx == 23 || idx == 24 || idx == 45)
        {
            if (error_offset != NULL)
            {
                *error_offset = at - taken + 1;
            }
            return SC_ERR_BAD_PHONEME;
        }
        if (a & ATTR_ILLEGAL)
        {
            if (error_offset != NULL)
            {
                *error_offset = at + 1;
            }
            return SC_ERR_BAD_PHONEME;
        }
        if (a & ATTR_PAUSE)
        {
            if (AttrOf(voice->attrs, s->phonemes[n - 1]) & ATTR_PAUSE)
            {
                if (n <= 4 || idx == 0)
                {
                    if (at >= length)
                    {
                        break;
                    }
                    s->stress[n] = s->flags[n] = 0;
                    continue;
                }
                --n;
            }
        }
        else if (idx == 5)
        {
            s->flags[n] |= 0x20;
            continue;
        }
        else if (idx == 6)
        {
            s->flags[n - 1] |= 0x10;
            continue;
        }

        s->phonemes[n++] = (uint8_t)idx;
        if (idx == 1 || idx == 2)
        {
            break;
        }
        if (n >= SC_MAX_PHONEMES)
        {
            if (error_offset != NULL)
            {
                *error_offset = at + 1;
            }
            return SC_ERR_BAD_PHONEME;
        }
        if (at >= length)
        {
            break;
        }
        s->stress[n] = s->flags[n] = 0;
    }

    if (s->phonemes[n - 1] == 0)
    {
        --n;
    }
    else if (AttrOf(voice->attrs, s->phonemes[n - 1]) & ATTR_TERMINAL)
    {
        dash = 0;
    }
    if (dash && n > 3)
    {
        s->phonemes[n] = 4;
        s->stress[n] = s->flags[n] = 0;
        ++n;
    }
    s->phonemes[n] = s->stress[n] = s->flags[n] = SC_TERMINATOR;
    s->count = ++n;
    s->consumed = at;
    if (n <= 4)
    {
        s->count = 0;
    }
    return SC_OK;

bad_here:
    if (error_offset != NULL)
    {
        *error_offset = at + 1;
    }
    return SC_ERR_BAD_PHONEME;
}

void SCMarkOnsets(struct SCPhonemes *s, const uint32_t *attrs)
{
    uint16_t i;
    for (i = 0; i < SC_ARRAY_SIZE; ++i)
    {
        uint8_t st = s->stress[i];
        uint32_t one, two;
        if (st == 0)
        {
            continue;
        }
        if (st == SC_TERMINATOR)
        {
            return;
        }
        if (i < 3)
        {
            continue;
        }
        one = AttrOf(attrs, s->phonemes[i - 1]);
        if (!(one & ATTR_CONSONANT))
        {
            continue;
        }
        s->stress[i - 1] |= 0x10;
        if (one & 0x28000)
        {
            two = AttrOf(attrs, s->phonemes[i - 2]);
            if (two & ATTR_VOICELESS_STOP)
            {
                s->stress[i - 2] |= 0x10;
                if (s->phonemes[i - 3] == 0x30)
                {
                    s->stress[i - 3] |= 0x10;
                }
            }
            else if (two & 0x5400)
            {
                s->stress[i - 2] |= 0x10;
            }
        }
        else if ((one & 0x10c00) && s->phonemes[i - 2] == 0x30)
        {
            s->stress[i - 2] |= 0x10;
        }
    }
}

static int TestGroup(const struct SCRewriteRule *r, unsigned *at,
                     uint8_t phoneme, uint8_t stress,
                     const uint8_t *tests, const uint32_t *attrs)
{
    while (*at < r->test_count)
    {
        uint8_t t = tests[r->test_first + (*at)++];
        uint32_t subject = (t & 0x40) ? AttrOf(attrs, phoneme) : stress;
        if (t & 0x20)
        {
            subject = ~subject;
        }
        if ((subject >> (t & 0x1f)) & 1)
        {
            return 0;
        }
        if (t & 0x80)
        {
            return 1;
        }
    }
    return 1;
}

static int RuleMatches(const struct SCPhonemes *s,
                       const struct SCRewriteRule *r, uint16_t i,
                       const uint8_t *tests, const uint32_t *attrs)
{
    uint16_t li = i - 1, ri = i + 1;
    uint8_t rp;
    unsigned t = 0;
    if (r->match != 0xff && r->match != s->phonemes[i])
    {
        return 0;
    }
    if (r->left != 0xff && r->left != s->phonemes[li])
    {
        return 0;
    }
    if (r->right != 0xff &&
            (s->phonemes[ri] == 0xff || r->right != s->phonemes[ri]))
    {
        return 0;
    }
    if (!TestGroup(r, &t, s->phonemes[i], s->stress[i], tests, attrs))
    {
        return 0;
    }
    if (s->phonemes[li] == 0 && (r->flags & 8))
    {
        --li;
    }
    if (!TestGroup(r, &t, s->phonemes[li], s->stress[li], tests, attrs))
    {
        return 0;
    }
    if (s->phonemes[ri] == 0 && (r->flags & 4))
    {
        ++ri;
    }
    rp = s->phonemes[ri] == 0xff ? 0 : s->phonemes[ri];
    return TestGroup(r, &t, rp, s->stress[ri], tests, attrs);
}

static int InsertAfter(struct SCPhonemes *s, uint16_t *i, uint8_t p)
{
    uint16_t k;
    if (s->count >= SC_MAX_PHONEMES)
    {
        return 0;
    }
    for (k = s->count; k > (uint16_t)(*i + 1); --k)
    {
        s->phonemes[k] = s->phonemes[k - 1];
        s->stress[k] = s->stress[k - 1];
        s->flags[k] = s->flags[k - 1];
    }
    ++s->count;
    ++*i;
    s->phonemes[*i] = p;
    s->stress[*i] = s->flags[*i] = 0;
    return 1;
}

int SCRewritePhonemes(struct SCPhonemes *s,
                        const struct SCRewriteRule *rules,
                        uint16_t rule_count, const uint8_t *tests,
                        const uint32_t *attrs)
{
    uint16_t i, r;
    for (i = 1; i <= SC_MAX_PHONEMES; ++i)
    {
        if (s->phonemes[i] == SC_TERMINATOR)
        {
            return SC_OK;
        }
        for (r = 0; r < rule_count; ++r)
        {
            const struct SCRewriteRule *rule = &rules[r];
            if (!RuleMatches(s, rule, i, tests, attrs))
            {
                continue;
            }
            if (rule->replace != 0xff)
            {
                s->phonemes[i] = rule->replace;
            }
            if (rule->insert_before != 0xff)
            {
                --i;
                if (!InsertAfter(s, &i, rule->insert_before))
                {
                    return SC_ERR_BAD_PHONEME;
                }
                ++i;
            }
            if (rule->insert_after != 0xff &&
                    !InsertAfter(s, &i, rule->insert_after))
            {
                return SC_ERR_BAD_PHONEME;
            }
            if (!(rule->flags & 2))
            {
                break;
            }
        }
    }
    return SC_OK;
}

static int DBRAOr(uint8_t *bytes, uint16_t *at, uint16_t count, uint8_t bits)
{
    uint16_t d = count;

    if (*at >= SC_ARRAY_SIZE ||
            (size_t)count + 1 > (size_t)SC_ARRAY_SIZE - *at)
    {
        return 0;
    }
    for (;;)
    {
        bytes[(*at)++] |= bits;
        --d;
        if (d == 0xffff)
        {
            return 1;
        }
    }
}

int SCSpreadStress(struct SCPhonemes *s, const uint32_t *attrs)
{
    uint16_t at = 1, span = 2, vowel = 0;
    uint8_t desc = 0;
    uint32_t last = 0;
    for (;;)
    {
        uint8_t p;
        int boundary;
        ++at;
        if (at >= SC_ARRAY_SIZE)
        {
            return SC_ERR_BAD_PHONEME;
        }
        p = s->phonemes[at];
        boundary = p == 0;
        if (!boundary)
        {
            if (p == SC_TERMINATOR)
            {
                return SC_OK;
            }
            last = AttrOf(attrs, p);
            boundary = (last & ATTR_TERMINAL) != 0;
        }
        if (!boundary)
        {
            uint16_t n;
            uint8_t first;
            if (!(last & ATTR_STRESSABLE) || p == 0x18 || p == 0x17)
            {
                continue;
            }
            if (vowel == 0)
            {
                vowel = at;
                continue;
            }
            n = (uint16_t)((((at - vowel) - 1) >> 1) + vowel - span);
            desc = (uint8_t)(((s->stress[vowel] & 0x10) << 1) | 0x40);
            if (span >= SC_ARRAY_SIZE)
            {
                return SC_ERR_BAD_PHONEME;
            }
            first = s->phonemes[span];
            if (first == 0x18 || first == 0x17)
            {
                s->stress[span++] |= desc;
                --n;
            }
            if (span >= SC_ARRAY_SIZE)
            {
                return SC_ERR_BAD_PHONEME;
            }
            s->stress[span] |= 0x80;
            if (!DBRAOr(s->stress, &span, n, desc))
            {
                return SC_ERR_BAD_PHONEME;
            }
            vowel = at;
            continue;
        }
        if (span >= SC_ARRAY_SIZE)
        {
            return SC_ERR_BAD_PHONEME;
        }
        if (s->phonemes[span] == 0x18 || s->phonemes[span] == 0x17)
        {
            ++span;
        }
        if (span >= SC_ARRAY_SIZE)
        {
            return SC_ERR_BAD_PHONEME;
        }
        s->stress[span] |= 0x80;
        if (vowel != 0)
        {
            uint16_t n = (uint16_t)(at - span - 1);
            s->flags[vowel] |= 0x80;
            desc = (uint8_t)((desc & ~0x20) |
                             ((s->stress[vowel] & 0x10) << 1));
            if (!DBRAOr(s->stress, &span, n, desc))
            {
                return SC_ERR_BAD_PHONEME;
            }
            if (last & ATTR_TERMINAL)
            {
                span = vowel;
                if (!DBRAOr(s->flags, &span,
                            (uint16_t)(at - vowel - 1), 0x40))
                {
                    return SC_ERR_BAD_PHONEME;
                }
            }
        }
        span = at + 1;
        vowel = 0;
        desc = 0;
    }
}

static uint16_t ScaleBy(uint16_t scale, uint16_t factor)
{
    return (uint16_t)((scale * factor + 0x10) >> 5);
}

static void StoreDuration(struct SCPhonemes *s, uint16_t i,
                          uint8_t old_flags, uint8_t duration)
{
    s->flags[i] = (uint8_t)((old_flags & 0xc0) | duration);
}

void SCAssignDurations(struct SCPhonemes *s,
                         const struct SCVoiceData *voice)
{
    uint16_t i = 2;
    const uint32_t *attrs = voice->attrs;
    for (; i + 2 < SC_ARRAY_SIZE;)
    {
        uint8_t st = s->stress[i], fl = s->flags[i], p = s->phonemes[i];
        uint32_t a, left, right;
        uint16_t scale = 0x20;
        uint8_t hi, lo, d;
        if (p == SC_TERMINATOR)
        {
            return;
        }
        a = AttrOf(attrs, p);
        if (a & (1UL << 20))
        {
            ++i;
            continue;
        }
        if (a & ATTR_TERMINAL)
        {
            StoreDuration(s, i++, fl, 0x18);
            continue;
        }
        if (fl & 0x40)
        {
            scale = ScaleBy(scale, 0x2d);
        }
        if ((a & 0x18000) && s->phonemes[i + 1] <= 8)
        {
            scale = ScaleBy(scale, 0x2d);
        }

        if (a & 1)
        {
            uint32_t rattr;
            if (!(fl & 0x80))
            {
                scale = ScaleBy(scale, 0x1b);
            }
            if (st & 0x40)
            {
                scale = ScaleBy(scale, 0x1a);
            }
            if (!(st & 0x10))
            {
                scale = ScaleBy(scale, 0x16);
            }
            rattr = AttrOf(attrs, s->phonemes[i + 1]);
            if (rattr & ((1UL << 20) | ATTR_TERMINAL))
            {
                if (st & 0x10)
                {
                    scale = ScaleBy(scale, 0x26);
                }
            }
            else if (rattr & (1UL << 12))
            {
                if (rattr & (1UL << 9))
                {
                    scale = ScaleBy(scale, 0x26);
                }
            }
            else if (rattr & (1UL << 10))
            {
                scale = ScaleBy(scale, 0x26);
            }
            else if (rattr & (1UL << 16))
            {
                if (!(s->stress[i + 1] & 0x10))
                {
                    scale = ScaleBy(scale, 0x1b);
                }
            }
            else if (rattr & (1UL << 11))
            {
                scale = ScaleBy(scale, 0x16);
            }
            else if (!(st & 0x10) && (a & 0x28000) &&
                     (rattr & 1))
            {
                scale = ScaleBy(scale, 3);
            }
        }
        else if (a & ATTR_CONSONANT)
        {
            if (s->phonemes[i - 1] <= 8)
            {
                scale = ScaleBy(scale, 0x1b);
            }
            if (!(st & 0x10) && (a & 0x28000) &&
                    (AttrOf(attrs, s->phonemes[i + 1]) & 1))
            {
                scale = ScaleBy(scale, 3);
            }
        }

        left = AttrOf(attrs, s->phonemes[i - 1] != 0 ?
                      s->phonemes[i - 1] : s->phonemes[i - 2]);
        right = AttrOf(attrs, s->phonemes[i + 1] != 0 ?
                       s->phonemes[i + 1] : s->phonemes[i + 2]);
        if (a & 1)
        {
            if (right & 1)
            {
                scale = ScaleBy(scale, 0x26);
            }
            if (left & 1)
            {
                scale = ScaleBy(scale, 0x16);
            }
        }
        else if (left & ATTR_CONSONANT)
        {
            scale = ScaleBy(scale, (right & ATTR_CONSONANT) ? 0x10 : 0x16);
        }
        else if (right & ATTR_CONSONANT)
        {
            scale = ScaleBy(scale, 0x16);
        }

        hi = voice->stressed[p];
        lo = voice->unstressed[p];
        if (!(st & 0x10) && !(a & 0x28000))
        {
            lo >>= 1;
        }
        d = (uint8_t)(hi - lo);
        d = (uint8_t)((((uint16_t)d * scale) & 0xffff) >> 5);
        d = (uint8_t)(d + lo);
        if ((a & 1) && (st & 0x10) &&
                (AttrOf(attrs, s->phonemes[i - 1]) & ATTR_VOICELESS_STOP))
        {
            d = (uint8_t)(d + 3);
        }
        if ((int8_t)d > 0x3f)
        {
            d = 0x3f;
        }
        StoreDuration(s, i++, fl, d);
    }
}
