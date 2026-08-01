#ifndef SPEECHCORE_H
#define SPEECHCORE_H

/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SC_OK                   0
#define SC_ERR_BAD_PHONEME    (-20)
#define SC_ERR_BAD_PARAM      (-21)
#define SC_ERR_NO_MEMORY      (-22)
#define SC_ERR_CANCELLED      (-23)
#define SC_ERR_OUTPUT         (-24)
#define SC_ERR_BAD_UTF8       (-25)
#define SC_ERR_UNSUPPORTED    (-26)
#define SC_ERR_RESOURCE       (-27)
#define SC_PCM_BLOCK_SIZE       512

#define SC_RESOURCE_VERSION      1
#define SC_RESOURCE_MAX_SIZE     (1024U * 1024U)

struct SCTranslatorData;
struct SCVoiceData;

extern const struct SCTranslatorData SCDefaultTranslator;
extern const struct SCVoiceData SCDefaultVoice;

struct SCResourceMetadata
{
    const char *generator;
    const char *translator_version;
    const char *translator_source;
    const char *translator_license;
    const char *voice_version;
    const char *voice_source;
    const char *voice_license;
};

struct SCResource
{
    struct SCTranslatorData *translator;
    struct SCVoiceData *voice;
    struct SCResourceMetadata metadata;
    /* Bytes actually occupied in the caller's decode workspace. */
    size_t workspace_used;
};

struct SCNarratorParams
{
    uint16_t rate;
    uint16_t pitch;
    uint16_t mode;
    uint16_t sex;
    uint16_t volume;
    uint16_t sample_frequency;
    uint8_t  mouths;
};

struct SCSink
{
    int (*pcm)(void *context, const int8_t *samples, size_t count,
               uint16_t period, uint16_t volume);
    int (*mouth)(void *context, const uint8_t *shapes, size_t count);
    int (*cancelled)(void *context);
    void *context;
    void *(*allocate)(void *context, size_t bytes);
    void (*release)(void *context, void *memory);
};

/* Classic Translate() semantics: NUL-terminate when output_size is nonzero. */
long SCTranslate(const char *input, size_t input_length,
                  char *output, size_t output_size);
long SCTranslateWith(const struct SCTranslatorData *tables,
                       const char *input, size_t input_length,
                       char *output, size_t output_size);

/* Validate and synthesize a bounded phoneme byte string. */
int SCSynth(const uint8_t *phonemes, size_t length,
             const struct SCNarratorParams *params,
             const struct SCSink *sink, size_t *error_offset);
int SCSynthWith(const struct SCVoiceData *voice,
                  const uint8_t *phonemes, size_t length,
                  const struct SCNarratorParams *params,
                  const struct SCSink *sink, size_t *error_offset);

/* Decode a big-endian IFF FORM NARR resource into caller-owned workspace. */
size_t SCResourceWorkspaceSize(size_t resource_length);
int SCResourceDecode(const uint8_t *bytes, size_t length,
                       void *workspace, size_t workspace_length,
                       struct SCResource *resource);

/* UTF-8 contract used by speech.device v1: valid ASCII subset only. */
int SCValidateEnglishUtf8(const uint8_t *text, size_t length,
                             size_t *error_offset);

void SCDefaultParams(struct SCNarratorParams *params);

/* 16.16 semitone offset to a 16.16 frequency ratio, clamped to +/-24. */
uint32_t SCSemitoneRatio(int32_t semitones);

#ifdef __cplusplus
}
#endif

#endif /* SPEECHCORE_H */
