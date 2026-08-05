/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include "speechcore.h"
#include "speech_data.h"
#include "synth_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Capture
{
    size_t samples;
    size_t mouths;
    unsigned nonzero;
    uint32_t hash;
};

static void *CaptureAlloc(void *context, size_t bytes)
{
    (void)context;
    return malloc(bytes);
}

static void CaptureFree(void *context, void *memory)
{
    (void)context;
    free(memory);
}

static int PCM(void *context, const int8_t *samples, size_t count,
               uint16_t period, uint16_t volume)
{
    struct Capture *capture = context;
    size_t i;
    if (period == 0 || volume > 64)
    {
        return 1;
    }
    capture->samples += count;
    for (i = 0; i < count; ++i)
    {
        if (samples[i] != 0)
        {
            capture->nonzero = 1;
        }
        capture->hash = (capture->hash * 16777619U) ^ (uint8_t)samples[i];
    }
    return 0;
}

static int Mouth(void *context, const uint8_t *shapes, size_t count)
{
    struct Capture *capture = context;
    (void)shapes;
    capture->mouths += count;
    return 0;
}

static uint8_t *FindChunk(uint8_t *bytes, size_t length, const char id[4],
                          size_t *chunk_length)
{
    size_t at;

    for (at = 12; at + 8 <= length;)
    {
        size_t size = ((size_t)bytes[at + 4] << 24) |
                      ((size_t)bytes[at + 5] << 16) |
                      ((size_t)bytes[at + 6] << 8) | bytes[at + 7];
        if (size > length - at - 8U)
        {
            return NULL;
        }
        if (memcmp(bytes + at, id, 4) == 0)
        {
            *chunk_length = size;
            return bytes + at + 8;
        }
        at += 8U + size + (size & 1U);
    }
    return NULL;
}

static int TestResource(const char *path)
{
    FILE *file = fopen(path, "rb");
    struct SCResource resource;
    struct SCNarratorParams params;
    struct Capture capture = { 0, 0, 0, 0 };
    struct SCSink sink = { PCM, Mouth, NULL, &capture,
               CaptureAlloc, CaptureFree
    };
    uint8_t *bytes = NULL;
    void *workspace = NULL;
    char phonemes[256];
    long length;
    size_t workspace_size;
    size_t workspace_used;
    size_t metadata_length;
    uint8_t *metadata, *table_version, *voice_fricatives, *voice_durations;
    uint8_t *voice_parameters, *voice_attributes;
    uint8_t *voice_rules;
    uint8_t saved, saved_stressed;
    size_t mutation;
    int result = 1;

    if (file == NULL || fseek(file, 0, SEEK_END) != 0 ||
            (length = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0)
    {
        goto out;
    }
    bytes = malloc((size_t)length);
    workspace_size = SCResourceWorkspaceSize((size_t)length);
    workspace = malloc(workspace_size);
    if (bytes == NULL || workspace == NULL ||
            fread(bytes, 1, (size_t)length, file) != (size_t)length)
    {
        goto out;
    }
    metadata = FindChunk(bytes, (size_t)length, "FVER", &metadata_length);
    if (metadata == NULL || metadata_length == 0)
    {
        goto out;
    }
    saved = metadata[0];
    metadata[0] = 0;
    if (SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                           &resource) == SC_OK)
    {
        goto out;
    }
    metadata[0] = saved;
    table_version = FindChunk(bytes, (size_t)length, "TVER",
                              &metadata_length);
    if (table_version == NULL)
    {
        goto out;
    }
    memcpy(table_version - 8, "FVER", 4);
    if (SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                           &resource) == SC_OK)
    {
        goto out;
    }
    memcpy(table_version - 8, "TVER", 4);
    if (SCResourceDecode(bytes, (size_t)length - 1U, workspace,
                           workspace_size, &resource) == SC_OK ||
            SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                               &resource) != SC_OK ||
            resource.translator == NULL || resource.voice == NULL ||
            resource.metadata.generator == NULL ||
            strncmp(resource.metadata.generator, "narrator-ts ", 12) != 0 ||
            resource.metadata.translator_version == NULL ||
            strcmp(resource.metadata.translator_version, "nrl-7948") != 0 ||
            resource.metadata.translator_source == NULL ||
            strcmp(resource.metadata.translator_source,
                   "NRL Report 7948 (1976-01-21)") != 0 ||
            resource.metadata.translator_license == NULL ||
            strcmp(resource.metadata.translator_license,
                   "Public domain; US Government work") != 0 ||
            resource.metadata.voice_version == NULL ||
            strcmp(resource.metadata.voice_version, "free") != 0 ||
            resource.metadata.voice_source == NULL ||
            strcmp(resource.metadata.voice_source,
                   "tools/gen-free-voice.py") != 0 ||
            resource.metadata.voice_license == NULL ||
            strcmp(resource.metadata.voice_license, "Public domain") != 0 ||
            SCTranslateWith(resource.translator, "hello world", 11, phonemes,
                              sizeof(phonemes)) != 0 ||
            strcmp(phonemes, "/HEH4LOW WER4LD ") != 0)
    {
        goto out;
    }
    workspace_used = resource.workspace_used;
    if (workspace_used == 0 || workspace_used >= workspace_size ||
            SCResourceDecode(bytes, (size_t)length, workspace,
                               workspace_used - 1U, &resource) == SC_OK ||
            SCResourceDecode(bytes, (size_t)length, workspace,
                               workspace_used, &resource) != SC_OK ||
            resource.workspace_used != workspace_used)
    {
        goto out;
    }
    voice_fricatives = FindChunk(bytes, (size_t)length, "VFRI",
                                 &metadata_length);
    if (voice_fricatives == NULL || metadata_length < 4)
    {
        fprintf(stderr, "missing VFRI\n");
        goto out;
    }
    saved = voice_fricatives[1];
    voice_fricatives[1] = 1;
    if (SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                           &resource) == SC_OK)
    {
        fprintf(stderr, "invalid VFRI accepted\n");
        goto out;
    }
    voice_fricatives[1] = saved;
    voice_durations = FindChunk(bytes, (size_t)length, "VDUR",
                                &metadata_length);
    if (voice_durations == NULL || metadata_length != 224)
    {
        fprintf(stderr, "invalid VDUR geometry: %lu\n", (unsigned long)metadata_length);
        goto out;
    }
    saved = voice_durations[112 + 9];
    saved_stressed = voice_durations[9];
    voice_durations[112 + 9] = 0;
    voice_durations[9] = 0;
    SCDefaultParams(&params);
    if (SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                           &resource) != SC_OK ||
            SCSynthWith(resource.voice, (const uint8_t *)"IY", 2, &params,
                         &sink, NULL) != SC_ERR_BAD_PHONEME)
    {
        fprintf(stderr, "zero VDUR reached frame construction\n");
        goto out;
    }
    voice_durations[112 + 9] = saved;
    voice_durations[9] = saved_stressed;
    saved = voice_durations[9];
    voice_durations[9] = 0x40;
    if (SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                           &resource) == SC_OK)
    {
        fprintf(stderr, "oversized VDUR accepted\n");
        goto out;
    }
    voice_durations[9] = saved;
    voice_parameters = FindChunk(bytes, (size_t)length, "VPRM",
                                 &metadata_length);
    if (voice_parameters == NULL || metadata_length != 112 * 12)
    {
        fprintf(stderr, "invalid VPRM geometry: %lu\n",
                (unsigned long)metadata_length);
        goto out;
    }
    saved = voice_parameters[112 * 9 + 11];
    voice_parameters[112 * 9 + 11] = 0x80;
    if (SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                           &resource) == SC_OK)
    {
        fprintf(stderr, "oversized VPRM transition accepted\n");
        goto out;
    }
    voice_parameters[112 * 9 + 11] = saved;
    voice_attributes = FindChunk(bytes, (size_t)length, "VATR",
                                 &metadata_length);
    if (voice_attributes == NULL || metadata_length != 112 * 4)
    {
        fprintf(stderr, "invalid VATR geometry: %lu\n",
                (unsigned long)metadata_length);
        goto out;
    }
    SCDefaultParams(&params);
    for (mutation = 0; mutation < metadata_length; ++mutation)
    {
        static const char *const probes[] = {
            "/HEH4LOW WER4LD ",
            "Q Q OWOYIN( L ",
            "DHAH KWIHK BROWN FAAKS JAHMPT OWVER DHAH LEYZIY DAOG."
        };
        static const uint8_t corrupt[] = { 0x00, 0xff };
        size_t probe;
        size_t value;

        saved = voice_attributes[mutation];
        for (value = 0; value < sizeof(corrupt); ++value)
        {
            voice_attributes[mutation] = corrupt[value];
            if (voice_attributes[mutation] == saved)
            {
                continue;
            }
            if (SCResourceDecode(bytes, (size_t)length, workspace,
                                   workspace_size, &resource) == SC_OK)
            {
                for (probe = 0;
                        probe < sizeof(probes) / sizeof(probes[0]); ++probe)
                {
                    memset(&capture, 0, sizeof(capture));
                    (void)SCSynthWith(resource.voice,
                        (const uint8_t *)probes[probe],
                        strlen(probes[probe]), &params, &sink, NULL);
                }
            }
        }
        voice_attributes[mutation] = saved;
    }
    voice_rules = FindChunk(bytes, (size_t)length, "VRL1",
                            &metadata_length);
    if (voice_rules == NULL || metadata_length < 10)
    {
        fprintf(stderr, "missing VRL1\n");
        goto out;
    }
    saved = voice_rules[6];
    voice_rules[6] = SC_VOICE_COUNT;
    if (SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                           &resource) == SC_OK)
    {
        fprintf(stderr, "invalid VRL1 accepted\n");
        goto out;
    }
    voice_rules[6] = saved;
    if (SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                           &resource) != SC_OK)
    {
        fprintf(stderr, "resource did not recover after mutations\n");
        goto out;
    }
    memset(&capture, 0, sizeof(capture));
    SCDefaultParams(&params);
    if (SCSynthWith(resource.voice, (const uint8_t *)phonemes,
                      strlen(phonemes), &params, &sink, NULL) != SC_OK ||
            capture.samples == 0 || !capture.nonzero)
    {
        goto out;
    }
    result = 0;
out:
    free(workspace);
    free(bytes);
    if (file != NULL)
    {
        fclose(file);
    }
    return result;
}

static int ExactCase(const char *input, struct SCNarratorParams *params,
                     size_t samples, uint32_t hash)
{
    struct Capture capture = { 0, 0, 0, 0 };
    struct SCSink sink = { PCM, Mouth, NULL, &capture,
               CaptureAlloc, CaptureFree
    };
    size_t error = 0;
    if (SCSynth((const uint8_t *)input, strlen(input), params, &sink,
                 &error) != SC_OK || capture.samples != samples ||
            capture.hash != hash)
    {
        fprintf(stderr, "exact case failed: %s samples=%lu hash=%lu error=%lu\n",
                input, (unsigned long)capture.samples,
                (unsigned long)capture.hash, (unsigned long)error);
        return 1;
    }
    return 0;
}

static int SafeCase(const char *input, struct SCNarratorParams *params)
{
    struct Capture capture = { 0, 0, 0, 0 };
    struct SCSink sink = { PCM, Mouth, NULL, &capture,
               CaptureAlloc, CaptureFree
    };
    size_t error = 0;
    int rc = SCSynth((const uint8_t *)input, strlen(input), params, &sink,
                      &error);
    if (rc != SC_OK || capture.samples == 0)
    {
        fprintf(stderr, "safety case failed: %s rc=%d error=%lu\n", input,
                rc, (unsigned long)error);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    char phonemes[256];
    char stressInput[361];
    char tiny[2] = { (char)0xaa, (char)0xaa };
    struct SCPhonemes stressState;
    struct SCNarratorParams params;
    struct Capture capture = { 0, 0, 0, 0 };
    struct SCSink sink = { PCM, Mouth, NULL, &capture,
               CaptureAlloc, CaptureFree
    };
    size_t error = 0;
    long rc;
    int32_t semitone;
    uint32_t previous = 0;
    unsigned i;

    if (SCSemitoneRatio(-32768) != 63697U ||
            SCSemitoneRatio(-65536) != 61858U ||
            SCSemitoneRatio(0) != 65536U ||
            SCSemitoneRatio(32768) != 67484U ||
            SCSemitoneRatio(65536) != 69433U ||
            SCSemitoneRatio(-(25L << 16)) !=
            SCSemitoneRatio(-(24L << 16)) ||
            SCSemitoneRatio(25L << 16) != SCSemitoneRatio(24L << 16))
    {
        fprintf(stderr, "semitone conversion mismatch\n");
        return 1;
    }
    for (semitone = -(24L << 16); semitone <= (24L << 16);
            semitone += 32768)
    {
        uint32_t ratio = SCSemitoneRatio(semitone);
        if (ratio < previous)
        {
            fprintf(stderr, "semitone conversion is not monotonic\n");
            return 1;
        }
        previous = ratio;
    }

    rc = SCTranslate("hello world", 11, phonemes, sizeof(phonemes));
    if (rc != 0 || strcmp(phonemes, "/HEH4LOW WER4LD ") != 0)
    {
        fprintf(stderr, "translate: rc=%ld, out=%s\n", rc, phonemes);
        return 1;
    }
    if (SCTranslate("a", 1, tiny, sizeof(tiny)) >= 0 || tiny[1] != 0)
    {
        fprintf(stderr, "translate overflow was not reported\n");
        return 1;
    }
    SCDefaultParams(&params);
    for (i = 0; i < 120; ++i)
        memcpy(stressInput + i * 3, "AA4", 3);
    stressInput[360] = 0;
    if (SCParsePhonemes(&stressState, &SCDefaultVoice,
                         (const uint8_t *)stressInput, 360, &error) != SC_OK)
    {
        fprintf(stderr, "stress boundary input did not parse\n");
        return 1;
    }
    SCMarkOnsets(&stressState, SCDefaultVoice.attrs);
    if (SCSpreadStress(&stressState, SCDefaultVoice.attrs) != SC_OK)
    {
        fprintf(stderr, "stress boundary input failed\n");
        return 1;
    }
    params.mouths = 1;
    if (SCSynth((const uint8_t *)phonemes, strlen(phonemes), &params,
                 &sink, &error) != 0 || capture.samples == 0 ||
            capture.mouths == 0 || !capture.nonzero)
    {
        fprintf(stderr, "synth: samples=%lu mouths=%lu nonzero=%u error=%lu\n",
                (unsigned long)capture.samples, (unsigned long)capture.mouths,
                capture.nonzero, (unsigned long)error);
        return 1;
    }
    if (capture.samples != 29184 || capture.mouths != 157 ||
            capture.hash != 3088334441U)
    {
        fprintf(stderr, "narrator-ts mismatch: samples=%lu mouths=%lu hash=%lu\n",
                (unsigned long)capture.samples, (unsigned long)capture.mouths,
                (unsigned long)capture.hash);
        return 1;
    }
    {
        uint32_t natural_hash = capture.hash;
        memset(&capture, 0, sizeof(capture));
        params.mode = 1;
        if (SCSynth((const uint8_t *)phonemes, strlen(phonemes), &params,
                     &sink, &error) != 0 || capture.hash == natural_hash ||
                capture.hash != 1629511665U)
        {
            fprintf(stderr, "robotic mode mismatch: hash=%lu\n",
                    (unsigned long)capture.hash);
            return 1;
        }
        params.mode = 0;
    }
    params.mouths = 0;
    if (ExactCase("DHAH KWIHK BROWN FAAKS JAHMPT OWVER DHAH LEYZIY DAOG.",
                  &params, 73728, 2868491494U) ||
            ExactCase("AY4 AEM SPIY4KIHNX. YUW4 LIXSAHN.",
                      &params, 52736, 2086358396U) ||
            ExactCase("IHZ DHIHS AH KWEH4SCHAHN?",
                      &params, 38400, 2278059527U))
    {
        return 1;
    }
    params.sex = 1;
    if (ExactCase("/HEHLOW WERLD ", &params, 26112, 3249555746U))
    {
        return 1;
    }
    params.sex = 0;
    params.rate = 40;
    params.pitch = 65;
    params.sample_frequency = 5000;
    if (ExactCase("/HEHLOW WERLD ", &params, 22528, 296964953U))
    {
        return 1;
    }
    params.rate = 400;
    params.pitch = 320;
    params.sample_frequency = 28000;
    if (ExactCase("/HEHLOW WERLD ", &params, 12288, 558532041U))
    {
        return 1;
    }
    SCDefaultParams(&params);
    if (SafeCase("L.", &params) || SafeCase("L?", &params) ||
            SafeCase("UXNUL", &params) ||
            SafeCase("Q Q OWOYIN( L ", &params) ||
            SafeCase("AX4AA-L", &params))
    {
        return 1;
    }
    if (SCSynth((const uint8_t *)"LX", 2, &params, &sink, &error)
            != SC_ERR_BAD_PHONEME || error != 1)
    {
        return 1;
    }
    if (SCValidateEnglishUtf8((const uint8_t *)"hello", 5, &error) != 0)
    {
        return 1;
    }
    if (SCValidateEnglishUtf8((const uint8_t *)"\xc3\xa9", 2, &error)
            != SC_ERR_UNSUPPORTED || error != 0)
    {
        return 1;
    }
    if (SCValidateEnglishUtf8((const uint8_t *)"\xe2\x28\xa1", 3, &error)
            != SC_ERR_BAD_UTF8 || error != 0)
    {
        return 1;
    }
    if (SCValidateEnglishUtf8(
                (const uint8_t *)"\xc3\xa9\xe2\x28\xa1", 5, &error)
            != SC_ERR_BAD_UTF8 || error != 2)
    {
        return 1;
    }
    if (argc > 1 && TestResource(argv[1]) != 0)
    {
        fprintf(stderr, "resource test failed: %s\n", argv[1]);
        return 1;
    }
    puts("speechcore host tests passed");
    return 0;
}
