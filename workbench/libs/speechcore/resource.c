/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include "speechcore.h"
#include "speech_data.h"

#include <stdint.h>
#include <string.h>

struct Reader
{
    const uint8_t *at;
    const uint8_t *end;
};

struct Arena
{
    uint8_t *at;
    uint8_t *end;
};

static int Take8(struct Reader *r, uint8_t *value)
{
    if (r->at == r->end)
    {
        return 0;
    }
    *value = *r->at++;
    return 1;
}

static int Take16(struct Reader *r, uint16_t *value)
{
    uint8_t a, b;
    if (!Take8(r, &a) || !Take8(r, &b))
    {
        return 0;
    }
    *value = (uint16_t)(((uint16_t)a << 8) | b);
    return 1;
}

static int Take32(struct Reader *r, uint32_t *value)
{
    uint8_t a, b, c, d;
    if (!Take8(r, &a) || !Take8(r, &b) || !Take8(r, &c) || !Take8(r, &d))
    {
        return 0;
    }
    *value = ((uint32_t)a << 24) | ((uint32_t)b << 16) |
             ((uint32_t)c << 8) | d;
    return 1;
}

static void *ArenaAlloc(struct Arena *arena, size_t size, size_t alignment)
{
    uintptr_t at = (uintptr_t)arena->at;
    uintptr_t aligned = (at + alignment - 1U) & ~(uintptr_t)(alignment - 1U);
    if (aligned > (uintptr_t)arena->end ||
            size > (size_t)((uintptr_t)arena->end - aligned))
    {
        return NULL;
    }
    arena->at = (uint8_t *)(aligned + size);
    return (void *)aligned;
}

static char *TakeString(struct Reader *r, struct Arena *arena)
{
    uint8_t length;
    char *out;
    if (!Take8(r, &length) || (size_t)(r->end - r->at) < length)
    {
        return NULL;
    }
    out = ArenaAlloc(arena, (size_t)length + 1U, 1);
    if (out == NULL)
    {
        return NULL;
    }
    memcpy(out, r->at, length);
    out[length] = 0;
    r->at += length;
    return out;
}

static int CountTranslatorRules(struct Reader r, uint16_t bucket_count,
                                size_t *result)
{
    size_t total = 0;
    uint16_t b, count, n;
    uint8_t length, ignored;
    unsigned p;

    for (b = 0; b < bucket_count; ++b)
    {
        if (!Take16(&r, &count) || total > 65535U - count)
        {
            return 0;
        }
        total += count;
        for (n = 0; n < count; ++n)
        {
            for (p = 0; p < 4; ++p)
            {
                if (!Take8(&r, &length) ||
                        (size_t)(r.end - r.at) < length)
                {
                    return 0;
                }
                r.at += length;
            }
            if (!Take8(&r, &ignored))
            {
                return 0;
            }
        }
    }
    if (r.at != r.end)
    {
        return 0;
    }
    *result = total;
    return 1;
}

static int DecodeTranslator(const uint8_t *bytes, size_t length,
                            struct Arena *arena,
                            struct SCTranslatorData **result)
{
    struct Reader r = { bytes, bytes + length };
    struct SCTranslatorData *data;
    struct SCTransRule *rules;
    struct SCTransBucket *buckets;
    uint16_t *classes;
    char (*vowels)[3];
    char *wildcards, *strings;
    uint16_t class_count, bucket_count, count, b, n;
    uint8_t vowel_count, term, stress_eligible = 0;
    size_t encoded_rules, rule_at = 0;

    data = ArenaAlloc(arena, sizeof(*data), sizeof(void *));
    if (data == NULL || !Take16(&r, &class_count) || class_count != 128)
    {
        return SC_ERR_RESOURCE;
    }
    classes = ArenaAlloc(arena, sizeof(*classes) * 128U, sizeof(uint16_t));
    if (classes == NULL)
    {
        return SC_ERR_RESOURCE;
    }
    for (n = 0; n < 128; ++n)
        if (!Take16(&r, &classes[n]))
        {
            return SC_ERR_RESOURCE;
        }

    wildcards = TakeString(&r, arena);
    if (wildcards == NULL || !Take8(&r, &vowel_count))
    {
        return SC_ERR_RESOURCE;
    }
    vowels = ArenaAlloc(arena, (size_t)vowel_count * 3U, 1);
    if (vowel_count != 0 && vowels == NULL)
    {
        return SC_ERR_RESOURCE;
    }
    for (n = 0; n < vowel_count; ++n)
    {
        char *value = TakeString(&r, arena);
        size_t size;
        if (value == NULL || (size = strlen(value)) != 2)
        {
            return SC_ERR_RESOURCE;
        }
        vowels[n][0] = size > 0 ? value[0] : 0;
        vowels[n][1] = size > 1 ? value[1] : 0;
        vowels[n][2] = 0;
    }
    if (!Take16(&r, &bucket_count) || bucket_count != 28)
    {
        return SC_ERR_RESOURCE;
    }
    buckets = ArenaAlloc(arena, sizeof(*buckets) * 28U,
                         sizeof(uint16_t));
    if (buckets == NULL)
    {
        return SC_ERR_RESOURCE;
    }

    if (!CountTranslatorRules(r, bucket_count, &encoded_rules))
    {
        return SC_ERR_RESOURCE;
    }
    rules = ArenaAlloc(arena, encoded_rules * sizeof(*rules),
                       sizeof(uint16_t));
    if (encoded_rules != 0 && rules == NULL)
    {
        return SC_ERR_RESOURCE;
    }
    strings = (char *)arena->at;
    for (b = 0; b < bucket_count; ++b)
    {
        if (!Take16(&r, &count) || rule_at + count > encoded_rules)
        {
            return SC_ERR_RESOURCE;
        }
        buckets[b].first = (uint16_t)rule_at;
        buckets[b].count = count;
        for (n = 0; n < count; ++n)
        {
            char *parts[4];
            unsigned p;
            for (p = 0; p < 4; ++p)
            {
                parts[p] = TakeString(&r, arena);
                if (parts[p] == NULL ||
                        (size_t)(parts[p] - strings) > 65535U)
                {
                    return SC_ERR_RESOURCE;
                }
            }
            if (!Take8(&r, &term))
            {
                return SC_ERR_RESOURCE;
            }
            if (parts[1][0] == 0)
            {
                if (b != 27 || n + 1 != count || parts[0][0] != 0 ||
                        parts[2][0] != 0 || strcmp(parts[3], " ") != 0 ||
                        term != '\\')
                {
                    return SC_ERR_RESOURCE;
                }
                --buckets[b].count;
                continue;
            }
            if (term != '`')
            {
                stress_eligible = 1;
            }
            rules[rule_at].left = (uint16_t)(parts[0] - strings);
            rules[rule_at].match = (uint16_t)(parts[1] - strings);
            rules[rule_at].right = (uint16_t)(parts[2] - strings);
            rules[rule_at].output = (uint16_t)(parts[3] - strings);
            rules[rule_at].term = term;
            ++rule_at;
        }
    }
    if (r.at != r.end || rule_at == 0 || (classes[' '] & (1U << 5)))
    {
        return SC_ERR_RESOURCE;
    }
    data->classes = classes;
    data->strings = strings;
    data->rules = rules;
    data->buckets = buckets;
    data->rule_count = (uint16_t)rule_at;
    data->wildcards = wildcards;
    data->vowels = (const char (*)[3])vowels;
    data->vowel_count = vowel_count;
    data->force_stress = vowel_count != 0 && !stress_eligible;
    *result = data;
    return SC_OK;
}

static uint8_t *TakeVector(struct Reader *r, struct Arena *arena, size_t count)
{
    uint8_t *out;
    if ((size_t)(r->end - r->at) < count)
    {
        return NULL;
    }
    out = ArenaAlloc(arena, count, 1);
    if (count != 0 && out == NULL)
    {
        return NULL;
    }
    memcpy(out, r->at, count);
    r->at += count;
    return out;
}

static int DecodeText(const uint8_t *bytes, size_t length,
                      struct Arena *arena, const char **result)
{
    char *text;

    if (bytes == NULL)
    {
        return SC_OK;
    }
    if (length == 0 || length > 255U || memchr(bytes, 0, length) != NULL)
    {
        return SC_ERR_RESOURCE;
    }
    text = ArenaAlloc(arena, length + 1U, 1);
    if (text == NULL)
    {
        return SC_ERR_RESOURCE;
    }
    memcpy(text, bytes, length);
    text[length] = 0;
    *result = text;
    return SC_OK;
}

struct VoiceChunks
{
    const uint8_t *names, *attrs, *params, *alt, *durations, *gain;
    const uint8_t *rules1, *rules2, *wave, *amp, *fricatives;
    size_t names_len, attrs_len, params_len, alt_len, durations_len, gain_len;
    size_t rules1_len, rules2_len, wave_len, amp_len, fricatives_len;
};

static int RuleShape(const uint8_t *bytes, size_t length,
                     uint16_t *rule_count, uint16_t *test_count)
{
    struct Reader r = { bytes, bytes + length };
    uint16_t count, n, tests = 0;
    if (!Take16(&r, &count) || count == 0)
    {
        return 0;
    }
    for (n = 0; n < count; ++n)
    {
        uint8_t ignored, amount;
        unsigned k;
        for (k = 0; k < 7; ++k) if (!Take8(&r, &ignored))
            {
                return 0;
            }
        if (!Take8(&r, &amount) || amount > (size_t)(r.end - r.at) ||
                tests > 65535U - amount)
        {
            return 0;
        }
        r.at += amount;
        tests = (uint16_t)(tests + amount);
    }
    if (r.at != r.end)
    {
        return 0;
    }
    *rule_count = count;
    *test_count = tests;
    return 1;
}

static int DecodeRules(const uint8_t *bytes, size_t length,
                       struct SCRewriteRule *rules, uint8_t *tests,
                       uint16_t test_base)
{
    struct Reader r = { bytes, bytes + length };
    uint16_t count, n, test_at = test_base;
    if (!Take16(&r, &count))
    {
        return 0;
    }
    for (n = 0; n < count; ++n)
    {
        uint8_t amount;
        if (!Take8(&r, &rules[n].match) || !Take8(&r, &rules[n].left) ||
                !Take8(&r, &rules[n].right) || !Take8(&r, &rules[n].flags) ||
                !Take8(&r, &rules[n].replace) ||
                !Take8(&r, &rules[n].insert_before) ||
                !Take8(&r, &rules[n].insert_after) || !Take8(&r, &amount))
        {
            return 0;
        }
        rules[n].test_first = test_at;
        rules[n].test_count = amount;
        memcpy(tests + test_at, r.at, amount);
        r.at += amount;
        test_at = (uint16_t)(test_at + amount);
    }
    return r.at == r.end;
}

static int DecodeVoice(const struct VoiceChunks *chunks,
                       struct Arena *arena, struct SCVoiceData **result)
{
    struct Reader rn = { chunks->names, chunks->names + chunks->names_len };
    struct Reader ra = { chunks->attrs, chunks->attrs + chunks->attrs_len };
    struct Reader rp = { chunks->params, chunks->params + chunks->params_len };
    struct Reader rl = { chunks->alt, chunks->alt + chunks->alt_len };
    struct Reader rd = { chunks->durations, chunks->durations + chunks->durations_len };
    struct Reader rf = { chunks->fricatives, chunks->fricatives + chunks->fricatives_len };
    struct Reader rg = { chunks->gain, chunks->gain + chunks->gain_len };
    struct Reader rw = { chunks->wave, chunks->wave + chunks->wave_len };
    struct Reader rm = { chunks->amp, chunks->amp + chunks->amp_len };
    struct SCVoiceData *data;
    struct SCRewriteRule *rules1, *rules2;
    uint8_t *tests;
    char (*names)[3];
    uint32_t *attrs, attr;
    uint16_t count, fricative_count, fricative_length, n;
    uint16_t rules1_count, rules2_count, tests1_count, tests2_count;
    size_t fricative_bytes;

    data = ArenaAlloc(arena, sizeof(*data), sizeof(void *));
    if (data == NULL || !Take16(&rn, &count) || count == 0 || count > 255 ||
            chunks->attrs_len != (size_t)count * 4U ||
            chunks->params_len != (size_t)count * 12U ||
            chunks->alt_len != (size_t)count * 3U ||
            chunks->durations_len != (size_t)count * 2U ||
            chunks->gain_len != 32 || chunks->wave_len != 4096 ||
            chunks->amp_len != 1024 ||
            !RuleShape(chunks->rules1, chunks->rules1_len,
                       &rules1_count, &tests1_count) ||
            !RuleShape(chunks->rules2, chunks->rules2_len,
                       &rules2_count, &tests2_count) ||
            tests1_count > 65535U - tests2_count)
    {
        return SC_ERR_RESOURCE;
    }
    memset(data, 0, sizeof(*data));
    names = ArenaAlloc(arena, (size_t)count * 3U, 1);
    attrs = ArenaAlloc(arena, (size_t)count * sizeof(*attrs), sizeof(uint32_t));
    if (names == NULL || attrs == NULL)
    {
        return SC_ERR_RESOURCE;
    }
    for (n = 0; n < count; ++n)
    {
        uint8_t a, b;
        if (!Take8(&rn, &a) || !Take8(&rn, &b))
        {
            return SC_ERR_RESOURCE;
        }
        names[n][0] = (char)a;
        names[n][1] = (char)b;
        names[n][2] = 0;
    }
    for (n = 0; n < count; ++n)
    {
        if (!Take32(&ra, &attr))
        {
            return SC_ERR_RESOURCE;
        }
        attrs[n] = attr;
    }
    data->f1 = TakeVector(&rp, arena, count);
    data->f2 = TakeVector(&rp, arena, count);
    data->f3 = TakeVector(&rp, arena, count);
    data->a1 = TakeVector(&rp, arena, count);
    data->a2 = TakeVector(&rp, arena, count);
    data->a3 = TakeVector(&rp, arena, count);
    data->voicing = TakeVector(&rp, arena, count);
    data->rank = TakeVector(&rp, arena, count);
    data->weight = TakeVector(&rp, arena, count);
    data->transition_in = TakeVector(&rp, arena, count);
    data->transition_out = TakeVector(&rp, arena, count);
    data->mouth = TakeVector(&rp, arena, count);
    data->alt_f1 = TakeVector(&rl, arena, count);
    data->alt_f2 = TakeVector(&rl, arena, count);
    data->alt_f3 = TakeVector(&rl, arena, count);
    data->stressed = TakeVector(&rd, arena, count);
    data->unstressed = TakeVector(&rd, arena, count);
    data->gain = TakeVector(&rg, arena, 32);
    data->wave = TakeVector(&rw, arena, 4096);
    data->amp = TakeVector(&rm, arena, 1024);
    if (data->unstressed == NULL || data->gain == NULL || data->wave == NULL ||
            data->amp == NULL || rn.at != rn.end || ra.at != ra.end ||
            rp.at != rp.end || rl.at != rl.end || rd.at != rd.end ||
            !Take16(&rf, &fricative_count) || !Take16(&rf, &fricative_length) ||
            fricative_count != SC_VOICE_FRICATIVE_COUNT ||
            fricative_length != SC_VOICE_FRICATIVE_LENGTH)
    {
        return SC_ERR_RESOURCE;
    }
    fricative_bytes = (size_t)fricative_count * fricative_length;
    data->fricatives = TakeVector(&rf, arena, fricative_bytes);
    rules1 = ArenaAlloc(arena, rules1_count * sizeof(*rules1), sizeof(uint16_t));
    rules2 = ArenaAlloc(arena, rules2_count * sizeof(*rules2), sizeof(uint16_t));
    tests = ArenaAlloc(arena, tests1_count + tests2_count, 1);
    if (data->fricatives == NULL || rf.at != rf.end || rules1 == NULL ||
            rules2 == NULL || tests == NULL ||
            !DecodeRules(chunks->rules1, chunks->rules1_len, rules1, tests, 0) ||
            !DecodeRules(chunks->rules2, chunks->rules2_len, rules2, tests,
                         tests1_count))
    {
        return SC_ERR_RESOURCE;
    }
    if (count != SC_VOICE_COUNT)
    {
        return SC_ERR_RESOURCE;
    }
    for (n = 0; n < count; ++n)
    {
        uint16_t other;
        if (data->a1[n] > 31 || data->a2[n] > 29 || data->a3[n] > 29 ||
                data->stressed[n] > 0x3f || data->unstressed[n] > 0x3f ||
                data->transition_in[n] > 0x3f ||
                data->transition_out[n] > 0x3f)
        {
            return SC_ERR_RESOURCE;
        }
        if (names[n][0] != 0)
            for (other = 0; other < n; ++other)
                if (names[n][0] == names[other][0] &&
                        names[n][1] == names[other][1])
                {
                    return SC_ERR_RESOURCE;
                }
    }
    for (n = 0; n < rules1_count + rules2_count; ++n)
    {
        const struct SCRewriteRule *rule = n < rules1_count
                                               ? &rules1[n] : &rules2[n - rules1_count];
        const uint8_t values[6] = { rule->match, rule->left, rule->right,
                                    rule->replace, rule->insert_before, rule->insert_after
                                  };
        unsigned k;
        for (k = 0; k < 6; ++k)
            if (values[k] != 0xff && values[k] >= count)
            {
                return SC_ERR_RESOURCE;
            }
        if ((uint32_t)rule->test_first + rule->test_count >
                (uint32_t)tests1_count + tests2_count)
        {
            return SC_ERR_RESOURCE;
        }
    }
    data->count = count;
    data->names = (const char (*)[3])names;
    data->attrs = attrs;
    data->gain_length = 32;
    data->allophone_rules = rules1;
    data->allophone_rule_count = rules1_count;
    data->frame_rules = rules2;
    data->frame_rule_count = rules2_count;
    data->rule_tests = tests;
    data->rule_test_count = (uint16_t)(tests1_count + tests2_count);
    data->wave_length = 4096;
    data->amp_length = 1024;
    data->fricative_count = fricative_count;
    data->fricative_length = fricative_length;
    *result = data;
    return SC_OK;
}

size_t SCResourceWorkspaceSize(size_t length)
{
    if (length < 12 || length > SC_RESOURCE_MAX_SIZE ||
            length > (SIZE_MAX - 4096U) / 4U)
    {
        return 0;
    }
    return length * 4U + 4096U;
}

static int IDIs(const uint8_t *p, const char id[4])
{
    return memcmp(p, id, 4) == 0;
}

int SCResourceDecode(const uint8_t *bytes, size_t length,
                       void *workspace, size_t workspace_length,
                       struct SCResource *resource)
{
    struct Arena arena;
    const uint8_t *translator = NULL;
    struct VoiceChunks voice;
    const uint8_t *generator = NULL;
    const uint8_t *translator_version = NULL, *translator_source = NULL;
    const uint8_t *translator_license = NULL, *voice_version = NULL;
    const uint8_t *voice_source = NULL, *voice_license = NULL;
    size_t translator_length = 0, at;
    size_t generator_length = 0;
    size_t translator_version_length = 0, translator_source_length = 0;
    size_t translator_license_length = 0, voice_version_length = 0;
    size_t voice_source_length = 0, voice_license_length = 0;
    uint32_t form_length, chunk_length, version = 0;
    int have_version = 0;
    int rc;

    memset(&voice, 0, sizeof(voice));

    if (bytes == NULL || workspace == NULL || workspace_length == 0 ||
            resource == NULL ||
            SCResourceWorkspaceSize(length) == 0 ||
            !IDIs(bytes, "FORM") || !IDIs(bytes + 8, "NARR"))
    {
        return SC_ERR_RESOURCE;
    }
    form_length = ((uint32_t)bytes[4] << 24) | ((uint32_t)bytes[5] << 16) |
                  ((uint32_t)bytes[6] << 8) | bytes[7];
    if ((size_t)form_length + 8U != length)
    {
        return SC_ERR_RESOURCE;
    }
    memset(resource, 0, sizeof(*resource));

    for (at = 12; at < length;)
    {
        const uint8_t *payload;
        if (length - at < 8)
        {
            return SC_ERR_RESOURCE;
        }
        chunk_length = ((uint32_t)bytes[at + 4] << 24) |
                       ((uint32_t)bytes[at + 5] << 16) |
                       ((uint32_t)bytes[at + 6] << 8) | bytes[at + 7];
        if ((size_t)chunk_length > length - at - 8U)
        {
            return SC_ERR_RESOURCE;
        }
        payload = bytes + at + 8;
        if (IDIs(bytes + at, "VERS"))
        {
            if (chunk_length != 4 || have_version)
            {
                return SC_ERR_RESOURCE;
            }
            version = ((uint32_t)payload[0] << 24) |
                      ((uint32_t)payload[1] << 16) |
                      ((uint32_t)payload[2] << 8) | payload[3];
            have_version = 1;
        }
        else if (IDIs(bytes + at, "FVER"))
        {
            if (generator != NULL)
            {
                return SC_ERR_RESOURCE;
            }
            generator = payload;
            generator_length = chunk_length;
        }
        else if (IDIs(bytes + at, "TVER"))
        {
            if (translator_version != NULL)
            {
                return SC_ERR_RESOURCE;
            }
            translator_version = payload;
            translator_version_length = chunk_length;
        }
        else if (IDIs(bytes + at, "TSRC"))
        {
            if (translator_source != NULL)
            {
                return SC_ERR_RESOURCE;
            }
            translator_source = payload;
            translator_source_length = chunk_length;
        }
        else if (IDIs(bytes + at, "TLIC"))
        {
            if (translator_license != NULL)
            {
                return SC_ERR_RESOURCE;
            }
            translator_license = payload;
            translator_license_length = chunk_length;
        }
        else if (IDIs(bytes + at, "LTRS"))
        {
            if (translator != NULL)
            {
                return SC_ERR_RESOURCE;
            }
            translator = payload;
            translator_length = chunk_length;
        }
        else if (IDIs(bytes + at, "VVER"))
        {
            if (voice_version != NULL)
            {
                return SC_ERR_RESOURCE;
            }
            voice_version = payload;
            voice_version_length = chunk_length;
        }
        else if (IDIs(bytes + at, "VSRC"))
        {
            if (voice_source != NULL)
            {
                return SC_ERR_RESOURCE;
            }
            voice_source = payload;
            voice_source_length = chunk_length;
        }
        else if (IDIs(bytes + at, "VLIC"))
        {
            if (voice_license != NULL)
            {
                return SC_ERR_RESOURCE;
            }
            voice_license = payload;
            voice_license_length = chunk_length;
        }
        else if (IDIs(bytes + at, "VNAM"))
        {
            if (voice.names)
            {
                return SC_ERR_RESOURCE;
            }
            voice.names = payload;
            voice.names_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VATR"))
        {
            if (voice.attrs)
            {
                return SC_ERR_RESOURCE;
            }
            voice.attrs = payload;
            voice.attrs_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VPRM"))
        {
            if (voice.params)
            {
                return SC_ERR_RESOURCE;
            }
            voice.params = payload;
            voice.params_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VALT"))
        {
            if (voice.alt)
            {
                return SC_ERR_RESOURCE;
            }
            voice.alt = payload;
            voice.alt_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VDUR"))
        {
            if (voice.durations)
            {
                return SC_ERR_RESOURCE;
            }
            voice.durations = payload;
            voice.durations_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VGAN"))
        {
            if (voice.gain)
            {
                return SC_ERR_RESOURCE;
            }
            voice.gain = payload;
            voice.gain_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VRL1"))
        {
            if (voice.rules1)
            {
                return SC_ERR_RESOURCE;
            }
            voice.rules1 = payload;
            voice.rules1_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VRL2"))
        {
            if (voice.rules2)
            {
                return SC_ERR_RESOURCE;
            }
            voice.rules2 = payload;
            voice.rules2_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VWAV"))
        {
            if (voice.wave)
            {
                return SC_ERR_RESOURCE;
            }
            voice.wave = payload;
            voice.wave_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VAMP"))
        {
            if (voice.amp)
            {
                return SC_ERR_RESOURCE;
            }
            voice.amp = payload;
            voice.amp_len = chunk_length;
        }
        else if (IDIs(bytes + at, "VFRI"))
        {
            if (voice.fricatives)
            {
                return SC_ERR_RESOURCE;
            }
            voice.fricatives = payload;
            voice.fricatives_len = chunk_length;
        }
        at += 8U + (size_t)chunk_length + (chunk_length & 1U);
        if (at > length)
        {
            return SC_ERR_RESOURCE;
        }
    }
    if (at != length || version != SC_RESOURCE_VERSION ||
            (translator == NULL && voice.names == NULL))
    {
        return SC_ERR_RESOURCE;
    }
    if (voice.names != NULL &&
            (voice.attrs == NULL || voice.params == NULL || voice.alt == NULL ||
             voice.durations == NULL || voice.gain == NULL || voice.rules1 == NULL ||
             voice.rules2 == NULL || voice.wave == NULL || voice.amp == NULL ||
             voice.fricatives == NULL))
    {
        return SC_ERR_RESOURCE;
    }

    arena.at = workspace;
    arena.end = arena.at + workspace_length;
    if ((rc = DecodeText(generator, generator_length, &arena,
                         &resource->metadata.generator)) != SC_OK ||
            (rc = DecodeText(translator_version, translator_version_length, &arena,
                             &resource->metadata.translator_version)) != SC_OK ||
            (rc = DecodeText(translator_source, translator_source_length, &arena,
                             &resource->metadata.translator_source)) != SC_OK ||
            (rc = DecodeText(translator_license, translator_license_length, &arena,
                             &resource->metadata.translator_license)) != SC_OK ||
            (rc = DecodeText(voice_version, voice_version_length, &arena,
                             &resource->metadata.voice_version)) != SC_OK ||
            (rc = DecodeText(voice_source, voice_source_length, &arena,
                             &resource->metadata.voice_source)) != SC_OK ||
            (rc = DecodeText(voice_license, voice_license_length, &arena,
                             &resource->metadata.voice_license)) != SC_OK)
    {
        return rc;
    }
    if (translator != NULL)
    {
        rc = DecodeTranslator(translator, translator_length, &arena,
                              &resource->translator);
        if (rc != SC_OK)
        {
            return rc;
        }
    }
    if (voice.names != NULL)
    {
        rc = DecodeVoice(&voice, &arena, &resource->voice);
        if (rc != SC_OK)
        {
            return rc;
        }
    }
    resource->workspace_used = (size_t)(arena.at - (uint8_t *)workspace);
    return SC_OK;
}
