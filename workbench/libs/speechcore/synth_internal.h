#ifndef SPEECH_SYNTH_INTERNAL_H
#define SPEECH_SYNTH_INTERNAL_H

/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include "speech_data.h"
#include <stddef.h>

#define SC_MAX_PHONEMES 0x200
#define SC_ARRAY_SIZE (SC_MAX_PHONEMES + 2)
#define SC_TERMINATOR 0xff

struct SCNarratorParams;
struct SCSink;

struct SCPhonemes
{
    uint8_t phonemes[SC_ARRAY_SIZE];
    uint8_t stress[SC_ARRAY_SIZE];
    uint8_t flags[SC_ARRAY_SIZE];
    uint16_t count;
    size_t consumed;
};

struct SCProsody
{
    uint8_t arr[8][0x80];
    uint16_t at_phoneme, at_stress, at_flag, arr_at;
    uint16_t pass, stresses, syllables, first, boundaries, last, total;
};

int SCParsePhonemes(struct SCPhonemes *state,
                      const struct SCVoiceData *voice,
                      const uint8_t *input, size_t length,
                      size_t *error_offset);
void SCMarkOnsets(struct SCPhonemes *state, const uint32_t *attrs);
int SCRewritePhonemes(struct SCPhonemes *state,
                        const struct SCRewriteRule *rules,
                        uint16_t rule_count,
                        const uint8_t *tests,
                        const uint32_t *attrs);
int SCSpreadStress(struct SCPhonemes *state, const uint32_t *attrs);
void SCAssignDurations(struct SCPhonemes *state,
                         const struct SCVoiceData *voice);
int SCBuildProsody(struct SCPhonemes *state, const uint32_t *attrs,
                     struct SCProsody *prosody, uint8_t mode);
int SCApplyContourWithAttrs(struct SCPhonemes *state,
                                const uint32_t *attrs,
                                struct SCProsody *prosody,
                                uint8_t *frames, size_t frame_bytes,
                                uint16_t pitch, uint8_t mode);
size_t SCFrameStorageSize(struct SCPhonemes *state,
                             const struct SCVoiceData *voice,
                             int mouths, size_t *total);
int SCBuildFrames(struct SCPhonemes *state,
                  const struct SCVoiceData *voice, int alternate,
                  uint8_t *frames, uint8_t *mouths, size_t total);
void SCFinishFrames(struct SCPhonemes *state,
                      const struct SCVoiceData *voice,
                      uint8_t *frames, uint8_t *mouths, size_t total);
int SCRenderFrames(uint8_t *frames, size_t frame_bytes,
                     const struct SCVoiceData *voice,
                     const struct SCNarratorParams *params,
                     const struct SCSink *sink);

#endif
