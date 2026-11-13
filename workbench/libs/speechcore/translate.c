/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include "speechcore.h"
#include "speech_data.h"

#include <string.h>

#define C_PUNCT       (1U << 0)
#define C_DIGIT       (1U << 1)
#define C_AFFECTS_U   (1U << 2)
#define C_VOICED      (1U << 3)
#define C_SIBILANT    (1U << 4)
#define C_CONSONANT   (1U << 5)
#define C_VOWEL       (1U << 6)
#define C_LETTER      (1U << 7)
#define C_FRONT       (1U << 8)
#define C_WILDCARD    (1U << 10)

#define SPACE         0x20
#define HASH          0x23
#define MAX_WORD      100
#define WORD_BUFFER   108
#define FILL_WORD_TOO_LONG 1

struct TranslatorState
{
    const struct SCTranslatorData *tables;
    const uint8_t *input;
    size_t input_length;
    size_t input_at;
    long remaining;
    char *output;
    size_t output_size;
    size_t output_at;
    size_t word_start;
    int stress_pending;
    int overflowed;
    uint8_t word[WORD_BUFFER];
    int position;
    int written;
};

static uint16_t ClassOf(const struct TranslatorState *s, unsigned c)
{
    return c < 128 ? s->tables->classes[c] : 0;
}

static uint8_t Normalise(uint8_t c)
{
    if (c >= 0x80 || c < 0x20 || c == 0x7f || c == ']')
    {
        return SPACE;
    }
    if (c >= 'a' && c <= 'z')
    {
        return (uint8_t)(c - ('a' - 'A'));
    }
    return c;
}

static void Emit(struct TranslatorState *s, uint8_t c)
{
    if (s->output_size == 0 || s->output_at >= s->output_size - 1)
    {
        s->overflowed = 1;
        return;
    }
    s->output[s->output_at++] = (char)c;
}

static void StressPass(struct TranslatorState *s)
{
    size_t i;
    uint8_t v;

    if (!s->stress_pending || s->output_at - s->word_start < 3)
    {
        s->word_start = s->output_at;
        return;
    }
    for (i = s->word_start; i < s->output_at; ++i)
        if (ClassOf(s, (uint8_t)s->output[i]) & C_DIGIT)
        {
            s->word_start = s->output_at;
            return;
        }
    for (i = s->word_start; i + 1 < s->output_at; ++i)
    {
        for (v = 0; v < s->tables->vowel_count; ++v)
        {
            if (s->output[i] == s->tables->vowels[v][0] &&
                    s->output[i + 1] == s->tables->vowels[v][1])
            {
                size_t tail = s->output_at - i - 2;
                if (s->output_at + 1 >= s->output_size)
                {
                    s->overflowed = 1;
                    break;
                }
                memmove(s->output + i + 3, s->output + i + 2, tail);
                s->output[i + 2] = '4';
                ++s->output_at;
                break;
            }
        }
        if (v < s->tables->vowel_count)
        {
            break;
        }
    }
    s->word_start = s->output_at;
}

static int FillWord(struct TranslatorState *s)
{
    int at = 0;
    uint8_t d0 = 0, d1 = 0;

    if (s->remaining <= 0)
    {
        Emit(s, HASH);
        s->written = 0;
        return 0;
    }
    s->word[at] = s->word[s->written < WORD_BUFFER ? s->written : 0];
    ++at;
    s->word[at++] = SPACE;
    s->written = 0;

    for (;;)
    {
        d0 = d1;
        d1 = s->input_at < s->input_length ? s->input[s->input_at++] : 0;
        if (d1 == 0)
        {
            s->remaining = 1;
        }
        d1 = Normalise(d1);
        ++s->written;
        if (s->written > MAX_WORD || at >= WORD_BUFFER - 2)
        {
            return FILL_WORD_TOO_LONG;
        }
        s->word[at++] = d1;

        if (d0 == SPACE)
        {
            if (s->input_at != 0)
            {
                --s->input_at;
            }
            --s->written;
            return 0;
        }
        --s->remaining;
        if (s->remaining == 0)
        {
            s->word[at++] = SPACE;
            s->word[at++] = SPACE;
            return 0;
        }
        if (d1 == HASH && d0 == HASH)
        {
            if (s->written == 2)
            {
                Emit(s, HASH);
                s->written = 0;
                return 0;
            }
            if (s->input_at != 0)
            {
                --s->input_at;
            }
            s->word[at - 2] = SPACE;
            --s->written;
            if (s->input_at != 0)
            {
                --s->input_at;
            }
            --s->written;
            return 0;
        }
    }
}

static uint8_t Step(struct TranslatorState *s, int direction)
{
    if (direction < 0)
    {
        if (s->position >= WORD_BUFFER)
        {
            return SPACE;
        }
        return s->word[s->position++];
    }
    if (s->position <= 0)
    {
        return SPACE;
    }
    return s->word[--s->position];
}

static uint16_t StepClass(struct TranslatorState *s, int direction)
{
    return ClassOf(s, Step(s, direction));
}

static int EndsWord(struct TranslatorState *s, int direction)
{
    return !(StepClass(s, direction) & C_LETTER);
}

static int Wildcard(struct TranslatorState *s, char wc, int direction)
{
    uint16_t c;
    uint8_t ch;

    switch (wc)
    {
    case '#':
        return !!(StepClass(s, direction) & C_VOWEL);
    case '*':
        if (!(StepClass(s, direction) & C_CONSONANT))
        {
            return 0;
        }
        /* Faithful: '*' does not Step back after the consonant run. */
        while (StepClass(s, direction) & C_CONSONANT) { }
        return 1;
    case '.':
        c = StepClass(s, direction);
        return !!((c & C_VOICED) && (c & C_CONSONANT));
    case '$':
        if (!(StepClass(s, direction) & C_CONSONANT))
        {
            return 0;
        }
        ch = Step(s, direction);
        return ch == 'I' || ch == 'E';
    case '%':
        ch = Step(s, direction);
        if (ch == 'E')
        {
            ch = Step(s, direction);
            if (!(ClassOf(s, ch) & C_LETTER))
            {
                return 1;
            }
            if (ch == 'S' || ch == 'D')
            {
                return EndsWord(s, direction);
            }
            if (ch == 'R')
            {
                return EndsWord(s, direction);
            }
            if (ch == 'L') return Step(s, direction)
                                      == 'Y' && EndsWord(s, direction);
            return 0;
        }
        if (Step(s, direction) != 'N')
        {
            return 0;
        }
        if (Step(s, direction) != 'G')
        {
            return 0;
        }
        return EndsWord(s, direction);
    case '&':
        return !!(StepClass(s, direction) & C_SIBILANT);
    case '@':
        return !!(StepClass(s, direction) & C_AFFECTS_U);
    case '^':
        return !!(StepClass(s, direction) & C_CONSONANT);
    case '+':
        return !!(StepClass(s, direction) & C_FRONT);
    case ':':
        while (StepClass(s, direction) & C_CONSONANT) { }
        s->position += direction;
        return 1;
    case '?':
        return !!(StepClass(s, direction) & C_DIGIT);
    case '_':
        while (StepClass(s, direction) & C_DIGIT) { }
        s->position += direction;
        return 1;
    default:
        return !(StepClass(s, direction) & C_LETTER);
    }
}

static int MatchContext(struct TranslatorState *s, const char *pattern,
                        int direction)
{
    size_t length = strlen(pattern), n;
    char ch;

    for (n = 0; n < length; ++n)
    {
        ch = direction > 0 ? pattern[length - n - 1] : pattern[n];
        if (ClassOf(s, (uint8_t)ch) & C_WILDCARD)
        {
            if (!Wildcard(s, ch, direction))
            {
                return 0;
            }
        }
        else if (Step(s, direction) != (uint8_t)ch)
        {
            return 0;
        }
    }
    return 1;
}

static int BucketFor(struct TranslatorState *s, uint8_t c)
{
    uint16_t cl = ClassOf(s, c);
    if (!(cl & C_LETTER))
    {
        c = (cl & C_DIGIT) ? '[' : '\\';
    }
    return (int)c - 'A';
}

static int TryRules(struct TranslatorState *s)
{
    int bucket;
    const struct SCTransBucket *b;
    uint16_t n;

    if (s->position < 0 || (size_t)s->position >= sizeof(s->word))
    {
        return 0;
    }
    bucket = BucketFor(s, s->word[s->position]);
    if (bucket < 0 || bucket >= 28)
    {
        return 0;
    }
    b = &s->tables->buckets[bucket];
    for (n = 0; n < b->count; ++n)
    {
        const struct SCTransRule *r = &s->tables->rules[b->first + n];
        const char *left = s->tables->strings + r->left;
        const char *match = s->tables->strings + r->match;
        const char *right = s->tables->strings + r->right;
        const char *out = s->tables->strings + r->output;
        int save = s->position;
        size_t match_length = strlen(match), i;

        if (save < 0 || (size_t)save >= sizeof(s->word) ||
                match_length > sizeof(s->word) - (size_t)save)
        {
            continue;
        }
        if (memcmp(s->word + save, match, match_length) != 0)
        {
            continue;
        }
        s->position = save;
        if (!MatchContext(s, left, 1))
        {
            s->position = save;
            continue;
        }
        s->position = save + (int)match_length;
        if (!MatchContext(s, right, -1))
        {
            s->position = save;
            continue;
        }
        s->position = save + (int)match_length;
        for (i = 0; out[i] != 0; ++i)
        {
            Emit(s, (uint8_t)out[i]);
        }
        if (s->output_at != 0 &&
                ClassOf(s, (uint8_t)s->output[s->output_at - 1]) & C_LETTER)
        {
            s->stress_pending = r->term != '`' || s->tables->force_stress;
        }
        return 1;
    }
    return 0;
}

long SCTranslateWith(const struct SCTranslatorData *tables,
                       const char *input, size_t input_length,
                       char *output, size_t output_size)
{
    struct TranslatorState s;
    int error;

    if (tables == NULL || (input == NULL && input_length != 0) ||
            output == NULL || output_size == 0)
    {
        return -(long)input_length;
    }
    memset(&s, 0, sizeof(s));
    s.tables = tables;
    s.input = (const uint8_t *)input;
    s.input_length = input_length;
    s.remaining = (long)input_length;
    s.output = output;
    s.output_size = output_size;
    memset(s.word, SPACE, sizeof(s.word));

    for (;;)
    {
        if (s.output_at != 0 && (uint8_t)s.output[s.output_at - 1] == HASH)
        {
            break;
        }
        if (s.position == 0 || s.word[s.position - 1] == SPACE)
        {
            error = FillWord(&s);
            StressPass(&s);
            if (error != 0)
            {
                output[s.output_at] = 0;
                {
                    size_t remaining = input_length -
                                       (s.input_at > input_length ? input_length : s.input_at);
                    return remaining != 0 ? -(long)remaining : -1;
                }
            }
            if (s.written == 0)
            {
                break;
            }
            s.position = 2;
        }
        if (!TryRules(&s))
        {
            ++s.position;
        }
        if (s.overflowed)
        {
            size_t remaining = input_length -
                               (s.input_at > input_length ? input_length : s.input_at);
            output[s.output_at] = 0;
            return remaining != 0 ? -(long)remaining : -1;
        }
    }
    StressPass(&s);
    if (s.output_at != 0 && (uint8_t)s.output[s.output_at - 1] == HASH)
    {
        --s.output_at;
    }
    output[s.output_at] = 0;
    return 0;
}

long SCTranslate(const char *input, size_t input_length,
                  char *output, size_t output_size)
{
    return SCTranslateWith(&SCDefaultTranslator, input, input_length,
                             output, output_size);
}
