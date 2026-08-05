/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include "speechcore.h"
#include "speech_data.h"
#include "synth_internal.h"

#include <string.h>
#ifdef SC_TRACE_FRAMES
#include <stdio.h>
#endif

static int ValidRules(const struct SCRewriteRule *rules, uint16_t count,
                      uint16_t phonemes, uint16_t tests)
{
    uint16_t n;
    for (n = 0; n < count; ++n)
    {
        const uint8_t values[6] = { rules[n].match, rules[n].left,
                                    rules[n].right, rules[n].replace, rules[n].insert_before,
                                    rules[n].insert_after
                                  };
        unsigned k;
        for (k = 0; k < 6; ++k)
            if (values[k] != 0xff && values[k] >= phonemes)
            {
                return 0;
            }
        if ((uint32_t)rules[n].test_first + rules[n].test_count > tests)
        {
            return 0;
        }
    }
    return 1;
}

static int ValidVoice(const struct SCVoiceData *v)
{
    uint16_t n;
    if (!(v != NULL && v->count != 0 && v->count <= 255 &&
            v->names != NULL && v->attrs != NULL &&
            v->f1 != NULL && v->f2 != NULL && v->f3 != NULL &&
            v->alt_f1 != NULL && v->alt_f2 != NULL && v->alt_f3 != NULL &&
            v->a1 != NULL && v->a2 != NULL && v->a3 != NULL &&
            v->voicing != NULL && v->rank != NULL && v->weight != NULL &&
            v->transition_in != NULL && v->transition_out != NULL &&
            v->mouth != NULL && v->stressed != NULL && v->unstressed != NULL &&
            v->gain != NULL && v->gain_length == 32 &&
            v->allophone_rules != NULL && v->allophone_rule_count != 0 &&
            v->frame_rules != NULL && v->frame_rule_count != 0 &&
            v->rule_tests != NULL && v->wave != NULL && v->wave_length == 4096 &&
            v->amp != NULL && v->amp_length == 1024 &&
            v->fricatives != NULL && v->fricative_count == 8 &&
            v->fricative_length == 480 &&
            ValidRules(v->allophone_rules, v->allophone_rule_count, v->count,
                       v->rule_test_count) &&
            ValidRules(v->frame_rules, v->frame_rule_count, v->count,
                       v->rule_test_count)))
    {
        return 0;
    }
    for (n = 0; n < v->count; ++n)
        if (v->a1[n] > 31 || v->a2[n] > 29 || v->a3[n] > 29 ||
                v->stressed[n] > 0x3f || v->unstressed[n] > 0x3f ||
                v->transition_in[n] > 0x3f ||
                v->transition_out[n] > 0x3f)
        {
            return 0;
        }
    return 1;
}

static int SynthSentence(const struct SCVoiceData *voice,
                         const uint8_t *input, size_t length,
                         const struct SCNarratorParams *params,
                         const struct SCSink *sink, size_t *consumed,
                         size_t *error_offset)
{
    struct SentenceWork
    {
        struct SCPhonemes state;
        struct SCProsody prosody;
    } *work;
    struct SCPhonemes *state;
    struct SCProsody *prosody;
    size_t total, bytes;
    uint8_t *storage = NULL, *frames, *mouths = NULL;
    uint16_t i;
    int rc;

    work = sink->allocate(sink->context, sizeof(*work));
    if (work == NULL)
    {
        return SC_ERR_NO_MEMORY;
    }
    state = &work->state;
    prosody = &work->prosody;
    rc = SCParsePhonemes(state, voice, input, length, error_offset);
    if (rc != SC_OK)
    {
        goto out;
    }
    *consumed = state->consumed;
    if (state->count == 0)
    {
        goto out;
    }

    SCMarkOnsets(state, voice->attrs);
    rc = SCRewritePhonemes(state, voice->allophone_rules,
                             voice->allophone_rule_count,
                             voice->rule_tests, voice->attrs);
    if (rc != SC_OK)
    {
        goto out;
    }
    rc = SCSpreadStress(state, voice->attrs);
    if (rc != SC_OK)
    {
        goto out;
    }
    rc = SCBuildProsody(state, voice->attrs, prosody,
                          (uint8_t)params->mode);
    if (rc != SC_OK)
    {
        goto out;
    }
#ifdef SC_TRACE_FRAMES
    {
        unsigned a, n;
        fprintf(stderr, "prosody total=%u\n", prosody->arr_at);
        for (a = 0; a < 8; ++a)
        {
            fprintf(stderr, "a%u=", a);
            for (n = 0; n < prosody->arr_at; ++n)
            {
                fprintf(stderr, "%s%u", n ? "," : "", prosody->arr[a][n]);
            }
            fputc('\n', stderr);
        }
    }
#endif
    SCAssignDurations(state, voice);
    rc = SCRewritePhonemes(state, voice->frame_rules,
                             voice->frame_rule_count,
                             voice->rule_tests, voice->attrs);
    if (rc != SC_OK)
    {
        goto out;
    }
    bytes = SCFrameStorageSize(state, voice, params->mouths, &total);
    if (bytes == 0)
    {
        rc = SC_ERR_NO_MEMORY;
        goto out;
    }
    for (i = 0; i < state->count && state->flags[i] != SC_TERMINATOR; ++i)
        if (state->phonemes[i] != 0 && (state->flags[i] & 0x3f) == 0)
        {
            rc = SC_ERR_BAD_PHONEME;
            if (error_offset != NULL)
            {
                *error_offset = state->consumed;
            }
            goto out;
        }
    storage = sink->allocate(sink->context, bytes);
    if (storage == NULL)
    {
        rc = SC_ERR_NO_MEMORY;
        goto out;
    }
    frames = storage;
    if (params->mouths)
    {
        mouths = frames + (total + 1) * 8;
    }
    memset(storage, 0, bytes);

    if (!SCBuildFrames(state, voice, params->sex != 0, frames, mouths,
                       total))
    {
        rc = SC_ERR_BAD_PHONEME;
        goto out;
    }
    rc = SCApplyContourWithAttrs(state, voice->attrs, prosody, frames,
                                     total * 8, params->pitch,
                                     (uint8_t)params->mode);
    if (rc != SC_OK)
    {
        goto out;
    }
    SCFinishFrames(state, voice, frames, mouths, total);
#ifdef SC_TRACE_FRAMES
    {
        uint32_t hash = 0;
        uint32_t columns[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        size_t trace_at;
        for (trace_at = 0; trace_at < (total + 1) * 8; ++trace_at)
        {
            hash = (hash * 16777619U) ^ frames[trace_at];
            columns[trace_at & 7] =
                (columns[trace_at & 7] * 16777619U) ^ frames[trace_at];
        }
        fprintf(stderr, "frames total=%lu hash=%lu cols=%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
                (unsigned long)total, (unsigned long)hash,
                (unsigned long)columns[0], (unsigned long)columns[1],
                (unsigned long)columns[2], (unsigned long)columns[3],
                (unsigned long)columns[4], (unsigned long)columns[5],
                (unsigned long)columns[6], (unsigned long)columns[7]);
    }
#endif
    if (mouths != NULL && sink->mouth != NULL &&
            sink->mouth(sink->context, mouths, total) != 0)
    {
        rc = SC_ERR_OUTPUT;
    }
    else
    {
        rc = SCRenderFrames(frames, (total + 1) * 8, voice, params, sink);
    }
out:
    if (storage != NULL)
    {
        sink->release(sink->context, storage);
    }
    sink->release(sink->context, work);
    return rc;
}

int SCSynthWith(const struct SCVoiceData *voice,
                  const uint8_t *phonemes, size_t length,
                  const struct SCNarratorParams *params,
                  const struct SCSink *sink, size_t *error_offset)
{
    struct SCNarratorParams defaults;
    size_t at = 0;
    if (error_offset != NULL)
    {
        *error_offset = 0;
    }
    if (!ValidVoice(voice) || sink == NULL || sink->allocate == NULL ||
            sink->release == NULL ||
            (phonemes == NULL && length != 0))
    {
        return SC_ERR_BAD_PARAM;
    }
    if (params == NULL)
    {
        SCDefaultParams(&defaults);
        params = &defaults;
    }
    if (params->rate < 40 || params->rate > 400 ||
            params->pitch < 65 || params->pitch > 320 ||
            params->sample_frequency < 5000 || params->sample_frequency > 28000 ||
            params->volume > 64 || params->mode > 2 || params->sex > 1)
    {
        return SC_ERR_BAD_PARAM;
    }
    while (at < length)
    {
        size_t used = 0, local_error = 0;
        int rc;
        if (sink->cancelled != NULL && sink->cancelled(sink->context))
        {
            return SC_ERR_CANCELLED;
        }
        rc = SynthSentence(voice, phonemes + at, length - at, params, sink,
                           &used, &local_error);
        if (rc != SC_OK)
        {
            if (error_offset != NULL)
            {
                *error_offset = local_error == 0 ? at : at + local_error;
            }
            return rc;
        }
        if (used == 0)
        {
            break;
        }
        at += used;
    }
    return SC_OK;
}

int SCSynth(const uint8_t *phonemes, size_t length,
             const struct SCNarratorParams *params,
             const struct SCSink *sink, size_t *error_offset)
{
    return SCSynthWith(&SCDefaultVoice, phonemes, length, params, sink,
                         error_offset);
}
