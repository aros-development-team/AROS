#include <config.h>

#include <aros/macros.h>

#include "rpihdmi-iec958.h"

/*
 * Map sample rate to IEC958 channel status byte 3 value.
 * IEC 60958-3 Table 3 (sampling frequency).
 */
static UBYTE srate_to_iec958_cs3(ULONG samplerate)
{
    switch (samplerate) {
    case 22050:
        return 0x04;
    case 44100:
        return 0x00;
    case 88200:
        return 0x08;
    case 176400:
        return 0x0C;
    case 24000:
        return 0x06;
    case 48000:
        return 0x02;
    case 96000:
        return 0x0A;
    case 192000:
        return 0x0E;
    case 32000:
        return 0x03;
    case 8000:
        return 0x06;
    case 11025:
        return 0x00;
    case 12000:
        return 0x02;
    case 16000:
        return 0x03;
    default:
        return 0x02;
    }
}

/******************************************************************************
** IEC958 channel status setup ************************************************
******************************************************************************/

/*
 * Set up IEC 60958-3 channel status for left and right channels.
 * Byte 2 bits 7:4 = channel number: 1=left, 2=right per IEC 60958-3.
 */
void spdif_setup_channel_status(UBYTE *cs_left, UBYTE *cs_right, ULONG samplerate)
{
    int i;

    for (i = 0; i < 24; i++)
        cs_left[i] = cs_right[i] = 0;

    /* Both channels share the same base settings */
    cs_left[0] = cs_right[0] = 0x04; /* Consumer, PCM, no copyright */
    cs_left[1] = cs_right[1] = 0x00; /* Category code = general */
    cs_left[3] = cs_right[3] = srate_to_iec958_cs3(samplerate);
    cs_left[4] = cs_right[4] = 0x02; /* 16-bit word length */

    /* Byte 2: bits 7:4 = channel number (1=left, 2=right) */
    cs_left[2] = 0x10;  /* Channel 1 (left) */
    cs_right[2] = 0x20; /* Channel 2 (right) */
}


/******************************************************************************
** IEC958 subframe encoding ***************************************************
******************************************************************************/

/*
 * Compute even parity for bits 4..30 of an IEC958 subframe.
 */
static inline ULONG iec958_parity(ULONG subframe)
{
    ULONG v = (subframe >> 4) & 0x07FFFFFF;
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return v & 1;
}

/*
 * Encode one IEC958 subframe (matching ALSA IEC958_SUBFRAME_LE format).
 *
 * Layout (32 bits):
 *   Bits 3:0   - Preamble (Z=0x08 block-start left, M=0x02 left, W=0x04 right)
 *   Bits 27:4  - 24-bit audio sample (16-bit left-shifted by 8)
 *   Bit 28     - Validity (0 = valid audio)
 *   Bit 29     - User data (0)
 *   Bit 30     - Channel status bit
 *   Bit 31     - Even parity over bits 4-30
 */
static inline ULONG encode_iec958_subframe(WORD sample, UBYTE cs_bit, UBYTE preamble)
{
    ULONG subframe;

    /* 16-bit sample -> 24-bit at bits 27:4 */
    subframe = ((ULONG) (UWORD) sample) << 12;

    /* Bits 3:0: preamble */
    subframe |= (preamble & 0x0F);

    /* Bit 28: validity = 0 (valid audio) */
    /* Bit 29: user data = 0 */

    /* Bit 30: channel status bit */
    if (cs_bit)
        subframe |= (1 << 30);

    /* Bit 31: even parity over bits 4-30 */
    if (iec958_parity(subframe))
        subframe |= (1 << 31);

    return subframe;
}

/*
 * Convert AHI mix buffer (signed 16-bit stereo) to IEC958 subframes.
 * Each stereo frame produces 2 x 32-bit subframes (left, right).
 * Channel status cycles through 192 frames per block.
 *
 * Preamble codes (matching ALSA alsa-lib pcm_iec958.c):
 *   Z = 0x08: block start, left channel (frame 0 of 192)
 *   M = 0x02: left channel (other frames)
 *   W = 0x04: right channel
 */
void convert_mix_to_iec958(WORD *src, ULONG *dst, ULONG frames, UBYTE *cs_left, UBYTE *cs_right, ULONG *frame_counter)
{
    ULONG i;
    ULONG fc = *frame_counter;

    for (i = 0; i < frames; i++) {
        WORD left = src[i * 2];
        WORD right = src[i * 2 + 1];
        UBYTE cs_bit_l = (cs_left[fc / 8] >> (fc % 8)) & 1;
        UBYTE cs_bit_r = (cs_right[fc / 8] >> (fc % 8)) & 1;
        UBYTE preamble_left = (fc == 0) ? 0x08 : 0x02;

        dst[i * 2] = AROS_LONG2LE(encode_iec958_subframe(left, cs_bit_l, preamble_left));
        dst[i * 2 + 1] = AROS_LONG2LE(encode_iec958_subframe(right, cs_bit_r, 0x04));

        fc++;
        if (fc >= 192)
            fc = 0;
    }

    *frame_counter = fc;
}
