/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include "speechcore.h"
#include "synth_internal.h"

#include <string.h>

#define WORD(v) ((uint16_t)(v))
#define HIGH(v) ((uint16_t)((v) >> 16))
#define AMP_MASK 0x3ffU
#if defined(__m68k__) && defined(__GNUC__)
static inline uint32_t SwapWords(uint32_t v)
{
    __asm__("swap %0" : "+d"(v));
    return v;
}

static uint32_t DecWord(uint32_t v)
{
    __asm__("subq.w #1,%0" : "+d"(v));
    return v;
}
#else
# define SwapWords(v) (((uint32_t)WORD(v) << 16) | HIGH(v))
static uint32_t DecWord(uint32_t v)
{
    return (v & 0xffff0000UL) | (uint16_t)(WORD(v) - 1);
}
#endif
#define SWAP(v) SwapWords(v)

struct Renderer
{
    uint8_t *frames;
    size_t length, fp;
    const struct SCVoiceData *voice;
    const struct SCNarratorParams *params;
    int8_t out[SC_PCM_BLOCK_SIZE];
    uint32_t d0, d1, d2, d4;
    uint16_t d3, f1inc;
    uint32_t f2f3inc;
    uint8_t *cur;
    uint8_t voicing;
    uint16_t noise_amp;
    const uint8_t *noise;
    uint16_t noise_index;
    int noise_step;
    uint16_t wave_at;
    uint8_t wave_count, mixed;
    uint16_t period_count, audio_period;
};

static void Pulse(struct Renderer *r)
{
    r->d1 = r->d2 = 0;
    r->wave_at = 0;
    r->wave_count = r->params->sex ? 9 : 11;
    r->d0 = (r->d0 & 0xffff0000UL) | r->cur[7];
    r->d3 = (uint16_t)r->cur[3] << 5;
    r->d4 = ((uint32_t)((uint16_t)r->cur[5] << 5) << 16) |
            ((uint16_t)r->cur[4] << 5);
}

static int Decode(struct Renderer *r)
{
    unsigned base;
    if (r->fp + 8 > r->length)
    {
        return 0;
    }
    r->cur = r->frames + r->fp;
    if (r->cur[0] & 0x80)
    {
        return 0;
    }
    r->f1inc = r->cur[0];
    r->f2f3inc = ((uint32_t)r->cur[1] << 16) |
                 ((uint16_t)r->cur[2] << 1);
    r->voicing = r->cur[6];
    r->noise = NULL;
    if (r->voicing != 0)
    {
        unsigned table = (r->voicing >> 4) & 7;
        if (r->voicing & 0x80)
        {
            r->mixed = 1;
        }
        else
        {
            r->d3 = 0;
        }
        r->noise_amp = (r->voicing & 15) << 5;
        if (table < r->voice->fricative_count)
            r->noise = r->voice->fricatives +
                       table * r->voice->fricative_length;
    }
    if (r->voicing > 0 && r->voicing < 0x80)
    {
        for (base = 0; base <= 8; base += 8)
            if (r->fp + base + 5 < r->length)
                r->frames[r->fp + base + 3] =
                    r->frames[r->fp + base + 4] =
                        r->frames[r->fp + base + 5] = 0;
    }
    r->fp += 8;
    return 1;
}

static int NextFrame(struct Renderer *r)
{
    if (!Decode(r))
    {
        return 0;
    }
    r->d0 = (r->d0 & 0xffff0000UL) | r->period_count;
    r->d0 = SWAP(r->d0);
    r->d0 = DecWord(r->d0);
    if ((int16_t)WORD(r->d0) < 0)
    {
        Pulse(r);
    }
    r->d0 = SWAP(r->d0);
    return 1;
}

#if defined(__m68k__) && defined(__GNUC__)
__attribute__((optimize("O3,no-strict-aliasing")))
#endif
int SCRenderFrames(uint8_t *frames, size_t frame_bytes,
                     const struct SCVoiceData *voice,
                     const struct SCNarratorParams *params,
                     const struct SCSink *sink)
{
    struct Renderer r;
    int done = 0;
    uint32_t n;
    /*
     * These are narrator.device's hot register set.  Keeping them as scalar
     * locals lets a 68k compiler retain them in D/A registers; accessing the
     * same values through the large Renderer object made GCC reload them from
     * its stack frame for almost every generated sample.
     */
#if defined(__m68k__) && defined(__GNUC__)
    register uint32_t d0 __asm__("d2");
    register uint32_t d1 __asm__("d3");
    register uint32_t d2 __asm__("d4");
    register uint32_t d4 __asm__("d5");
    const uint8_t *wave_window, *amp;
    int8_t *out;
    uint8_t *cur;
#else
    uint32_t d0, d1, d2, d4;
    const uint8_t *wave_window, *amp;
    int8_t *out;
    uint8_t *cur;
#endif
    uint32_t f2f3inc;
    uint16_t d3, f1inc, noise_amp, noise_index;
    const uint8_t *noise;
    uint8_t voicing, wave_count, wave_span, mixed;
    int noise_step;
    unsigned used = 0;
    memset(&r, 0, sizeof(r));
    r.frames = frames;
    r.length = frame_bytes;
    r.voice = voice;
    r.params = params;
    n = (uint32_t)params->sample_frequency * 0x4bUL;
    n = (uint16_t)(n / params->rate);
    r.period_count = (uint16_t)((n / 0x3c) >> 1);
    r.audio_period = (uint16_t)(0x369c78UL / params->sample_frequency);
    wave_span = params->sex ? 9 : 11;
    r.noise_step = 1;
    r.d0 = (uint32_t)r.period_count << 16;
    if (!Decode(&r))
    {
        return SC_OK;
    }
    Pulse(&r);
    r.d0 = SWAP(r.d0);

#define LOAD_HOT_STATE() do { \
    d0 = r.d0; d1 = r.d1; d2 = r.d2; d3 = r.d3; d4 = r.d4; \
    f1inc = r.f1inc; f2f3inc = r.f2f3inc; cur = r.cur; \
    voicing = r.voicing; noise_amp = r.noise_amp; noise = r.noise; \
    noise_index = r.noise_index; noise_step = r.noise_step; \
    wave_window = voice->wave + r.wave_at; \
    wave_count = r.wave_count; mixed = r.mixed; \
} while (0)
#define STORE_HOT_STATE() do { \
    r.d0 = d0; r.d1 = d1; r.d2 = d2; r.d3 = d3; r.d4 = d4; \
    r.f1inc = f1inc; r.f2f3inc = f2f3inc; r.cur = cur; \
    r.voicing = voicing; r.noise_amp = noise_amp; r.noise = noise; \
    r.noise_index = noise_index; r.noise_step = noise_step; \
    r.wave_at = (uint16_t)(wave_window - voice->wave); \
    r.wave_count = wave_count; r.mixed = mixed; \
} while (0)
#define ADVANCE_FRAME() do { \
    STORE_HOT_STATE(); \
    if (!NextFrame(&r)) done = 1; \
    LOAD_HOT_STATE(); \
} while (0)
#define EMIT_VALUES(a, b) do { \
    int emit_rc; \
    out[used++] = (int8_t)(a); \
    out[used++] = (int8_t)(b); \
    if (used == SC_PCM_BLOCK_SIZE) { \
        STORE_HOT_STATE(); \
        if (sink->cancelled != NULL && sink->cancelled(sink->context)) \
            return SC_ERR_CANCELLED; \
        emit_rc = sink->pcm == NULL ? 0 : \
            sink->pcm(sink->context, out, SC_PCM_BLOCK_SIZE, \
                      r.audio_period, params->volume); \
        LOAD_HOT_STATE(); \
        used = 0; \
        if (emit_rc != 0) return SC_ERR_OUTPUT; \
    } \
} while (0)
#define LOCAL_PULSE() do { \
    d1 = d2 = 0; \
    wave_window = voice->wave; \
    wave_count = wave_span; \
    d0 = (d0 & 0xffff0000UL) | cur[7]; \
    d3 = (uint16_t)cur[3] << 5; \
    d4 = ((uint32_t)((uint16_t)cur[5] << 5) << 16) | \
         ((uint16_t)cur[4] << 5); \
} while (0)

    LOAD_HOT_STATE();
    amp = voice->amp;
    out = r.out;

dispatch:
    if (done)
    {
        goto render_done;
    }
    if (voicing == 0)
    {
        mixed = 0;
        goto harmonic_loop;
    }
    if (!mixed)
    {
        goto noise_loop;
    }
    goto harmonic_loop;

harmonic_loop:
    {
        uint8_t first, second;
        if (voicing == 0)
        {
            uint8_t acc;
            uint16_t v;
            v = wave_window[(WORD(d1) >> 4) & 0x3f] | d3;
            acc = amp[v & AMP_MASK];
            v = wave_window[(d2 >> 4) & 0xfff] | WORD(d4);
            acc = (uint8_t)(acc + amp[v & AMP_MASK]);
            v = wave_window[(HIGH(d2 >> 4)) & 0xfff] | HIGH(d4);
            acc = (uint8_t)(acc + amp[v & AMP_MASK]);
            first = second = acc;
        }
        else
        {
            uint8_t b = noise == NULL ? 0 : noise[noise_index];
            uint8_t voiced, s1, s2, w;
            d4 = 0;
            w = wave_window[(WORD(d1) >> 4) & 0x3f] & 0x1f;
            voiced = amp[(w | d3) & AMP_MASK];
            s1 = (uint8_t)(voiced + amp[(((b & 15) << 1) | noise_amp) &
                                        AMP_MASK]);
            s2 = (uint8_t)(amp[(((b & 0xf0) >> 3) | noise_amp) &
                               AMP_MASK] + voiced);
            first = s1;
            second = s2;
            noise_index = (uint16_t)(noise_index + noise_step);
            if (noise_index == 0 || noise_index == 0x1df)
            {
                noise_step = -noise_step;
            }
        }
        EMIT_VALUES(first, second);
        if (voicing == 0)
        {
            d1 = WORD(d1 + f1inc) & 0x3ff;
            d2 = SWAP(SWAP(d2) + f2f3inc) & 0x03ff03ffUL;
            if (--wave_count == 0)
            {
                wave_window += 0x40;
                wave_count = wave_span;
            }
        }
        else
        {
            d1 = WORD(d1 + f1inc) & 0x3ff;
            d2 = 0;
        }
        d0 = DecWord(d0);
        if ((int16_t)WORD(d0) < 0)
        {
            ADVANCE_FRAME();
            goto dispatch;
        }
        d0 = SWAP(d0);
        d0 = DecWord(d0);
        if ((int16_t)WORD(d0) < 0)
        {
            LOCAL_PULSE();
        }
        d0 = SWAP(d0);
        goto harmonic_loop;
    }

noise_loop:
    {
        uint8_t b = noise == NULL ? 0 : noise[noise_index];
        uint8_t s1, s2;
        d4 = 0;
        /* The nibble offsets occupy bits below noise_amp, so addition is the
         * same index as narrator's OR without needing a trailing mask. */
        s1 = amp[noise_amp + ((b & 15) << 1)];
        s2 = amp[noise_amp + ((b & 0xf0) >> 3)];
        EMIT_VALUES(s1, s2);
        noise_index = (uint16_t)(noise_index + noise_step);
        if (noise_index == 0 || noise_index == 0x1df)
        {
            noise_step = -noise_step;
        }
        d0 = DecWord(d0);
        if ((int16_t)WORD(d0) < 0)
        {
            ADVANCE_FRAME();
            goto dispatch;
        }
        goto noise_loop;
    }

render_done:
#undef LOCAL_PULSE
#undef EMIT_VALUES
#undef ADVANCE_FRAME
#undef STORE_HOT_STATE
#undef LOAD_HOT_STATE
    return SC_OK;
}
