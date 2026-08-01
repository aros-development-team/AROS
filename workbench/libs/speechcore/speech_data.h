#ifndef SPEECH_DATA_H
#define SPEECH_DATA_H

/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <stdint.h>

struct SCTransRule
{
    uint16_t left;
    uint16_t match;
    uint16_t right;
    uint16_t output;
    uint8_t term;
};

struct SCTransBucket
{
    uint16_t first;
    uint16_t count;
};

struct SCTranslatorData
{
    const uint16_t *classes;
    const char *strings;
    const struct SCTransRule *rules;
    const struct SCTransBucket *buckets;
    uint16_t rule_count;
    const char *wildcards;
    const char (*vowels)[3];
    uint8_t vowel_count;
    /* Public-domain NRL tables have no per-rule stress eligibility. */
    uint8_t force_stress;
};

/* narrator.device's compact, table-driven rewrite rule.  Test bytes live in
   the voice-wide rule_tests vector and are addressed by this slice. */
struct SCRewriteRule
{
    uint8_t match;
    uint8_t left;
    uint8_t right;
    uint8_t flags;
    uint8_t replace;
    uint8_t insert_before;
    uint8_t insert_after;
    uint16_t test_first;
    uint8_t test_count;
};

extern const uint16_t SCTransClasses[128];
extern const char SCTransStrings[];
extern const struct SCTransRule SCTransRules[];
extern const struct SCTransBucket SCTransBuckets[28];
extern const uint16_t SCTransRuleCount;
extern const char SCTransWildcards[];
extern const char SCTransVowels[][3];
extern const uint8_t SCTransVowelCount;

#define SC_VOICE_COUNT 112
#define SC_VOICE_ATTR_COUNT 102
#define SC_VOICE_OPEN_PAREN 5
#define SC_VOICE_CLOSE_PAREN 6
#define SC_VOICE_FRICATIVE_COUNT 8
#define SC_VOICE_FRICATIVE_LENGTH 480
#define SC_VOICE_ALLOPHONE_RULE_COUNT 7
#define SC_VOICE_FRAME_RULE_COUNT 42
#define SC_VOICE_RULE_TEST_COUNT 153

extern const char SCVoiceNames[SC_VOICE_COUNT][3];
extern const uint32_t SCVoiceAttrs[SC_VOICE_COUNT];
extern const uint8_t SCVoiceF1[SC_VOICE_COUNT];
extern const uint8_t SCVoiceF2[SC_VOICE_COUNT];
extern const uint8_t SCVoiceF3[SC_VOICE_COUNT];
extern const uint8_t SCVoiceAltF1[SC_VOICE_COUNT];
extern const uint8_t SCVoiceAltF2[SC_VOICE_COUNT];
extern const uint8_t SCVoiceAltF3[SC_VOICE_COUNT];
extern const uint8_t SCVoiceA1[SC_VOICE_COUNT];
extern const uint8_t SCVoiceA2[SC_VOICE_COUNT];
extern const uint8_t SCVoiceA3[SC_VOICE_COUNT];
extern const uint8_t SCVoiceVoicing[SC_VOICE_COUNT];
extern const uint8_t SCVoiceRank[SC_VOICE_COUNT];
extern const uint8_t SCVoiceWeight[SC_VOICE_COUNT];
extern const uint8_t SCVoiceTransitionIn[SC_VOICE_COUNT];
extern const uint8_t SCVoiceTransitionOut[SC_VOICE_COUNT];
extern const uint8_t SCVoiceMouth[SC_VOICE_COUNT];
extern const uint8_t SCVoiceStressed[SC_VOICE_COUNT];
extern const uint8_t SCVoiceUnstressed[SC_VOICE_COUNT];
extern const uint8_t SCVoiceGain[32];
extern const struct SCRewriteRule SCVoiceAllophoneRules[];
extern const uint16_t SCVoiceAllophoneRuleCount;
extern const struct SCRewriteRule SCVoiceFrameRules[];
extern const uint16_t SCVoiceFrameRuleCount;
extern const uint8_t SCVoiceRuleTests[];
extern const uint16_t SCVoiceRuleTestCount;
extern const uint8_t SCVoiceWave[4096];
extern const uint8_t SCVoiceAmp[1024];
extern const uint8_t SCVoiceFricatives[SC_VOICE_FRICATIVE_COUNT][SC_VOICE_FRICATIVE_LENGTH];

struct SCVoiceData
{
    uint16_t count;
    const char (*names)[3];
    const uint32_t *attrs;
    const uint8_t *f1;
    const uint8_t *f2;
    const uint8_t *f3;
    const uint8_t *alt_f1;
    const uint8_t *alt_f2;
    const uint8_t *alt_f3;
    const uint8_t *a1;
    const uint8_t *a2;
    const uint8_t *a3;
    const uint8_t *voicing;
    const uint8_t *rank;
    const uint8_t *weight;
    const uint8_t *transition_in;
    const uint8_t *transition_out;
    const uint8_t *mouth;
    const uint8_t *stressed;
    const uint8_t *unstressed;
    const uint8_t *gain;
    uint16_t gain_length;
    const struct SCRewriteRule *allophone_rules;
    uint16_t allophone_rule_count;
    const struct SCRewriteRule *frame_rules;
    uint16_t frame_rule_count;
    const uint8_t *rule_tests;
    uint16_t rule_test_count;
    const uint8_t *wave;
    uint16_t wave_length;
    const uint8_t *amp;
    uint16_t amp_length;
    uint16_t fricative_count;
    uint16_t fricative_length;
    const uint8_t *fricatives;
};

extern const struct SCTranslatorData SCDefaultTranslator;
extern const struct SCVoiceData SCDefaultVoice;

#endif /* SPEECH_DATA_H */
