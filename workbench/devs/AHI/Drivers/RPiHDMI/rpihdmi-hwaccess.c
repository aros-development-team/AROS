/*
 *  BCM2835 HDMI Audio hardware access for Raspberry Pi
 *
 *  Configures the HDMI MAI (Multi-channel Audio Interconnect) for
 *  audio output via HDMI, using DMA to feed IEC958 subframes
 *  to the MAI DATA FIFO.
 *
 *  The MAI passes IEC958 subframes through with minimal processing.
 *  Software must provide fully formatted subframes (audio data,
 *  validity, user data, channel status, parity).
 *
 *  Register offsets taken from Linux vc4_hdmi_fields[] for BCM2835.
 *  No VCHIQ firmware interaction needed — HDMI link is already
 *  established by the VideoCore firmware at boot.
 */

#include <config.h>

#include <exec/types.h>
#include <aros/macros.h>

#include "rpihdmi-hwaccess.h"

/*
 * Microsecond delay using a busy loop on the system timer.
 */
static void udelay(ULONG peribase, ULONG us)
{
    volatile ULONG *clo = (volatile ULONG *) (ULONG) (peribase + 0x003004);
    ULONG start = AROS_LE2LONG(*clo);

    while ((AROS_LE2LONG(*clo) - start) < us)
        ;
}

/*
 * Map sample rate to MAI format enum value.
 */
static ULONG srate_to_mai_enum(ULONG samplerate)
{
    switch (samplerate) {
    case 8000:
        return SRATE_8000;
    case 11025:
        return SRATE_11025;
    case 12000:
        return SRATE_12000;
    case 16000:
        return SRATE_16000;
    case 22050:
        return SRATE_22050;
    case 24000:
        return SRATE_24000;
    case 32000:
        return SRATE_32000;
    case 44100:
        return SRATE_44100;
    case 48000:
        return SRATE_48000;
    case 88200:
        return SRATE_88200;
    case 96000:
        return SRATE_96000;
    case 176400:
        return SRATE_176400;
    case 192000:
        return SRATE_192000;
    default:
        return SRATE_48000;
    }
}

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

/*
 * Get N value for HDMI audio clock recovery.
 * Standard N values from HDMI spec Table 7-1/7-2/7-3.
 */
static ULONG srate_to_n(ULONG samplerate)
{
    switch (samplerate) {
    case 32000:
        return 4096;
    case 44100:
        return 6272;
    case 48000:
        return 6144;
    case 88200:
        return 12544;
    case 96000:
        return 12288;
    case 176400:
        return 25088;
    case 192000:
        return 24576;
    default:
        return 128 * samplerate / 1000;
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


/******************************************************************************
** HDMI Audio InfoFrame *******************************************************
******************************************************************************/

/*
 * Write a minimal HDMI Audio InfoFrame to RAM packet memory.
 * The infoframe tells the TV: 2-channel LPCM, sample rate, 16-bit.
 *
 * Audio InfoFrame (CEA-861):
 *   Type = 0x84, Version = 1, Length = 10
 *   Byte 1: CC=1 (2ch), CT=1 (PCM)
 *   Byte 2: SS=1 (16-bit), SF (sample rate code)
 *   Bytes 3-10: 0
 *
 * RAM packet layout: each packet slot is 0x24 bytes.
 * Audio InfoFrame is type 0x84, packet_id = type - 0x80 = 4.
 * Slot offset = 0x400 + packet_id * 0x24 = 0x490.
 */
static UBYTE srate_to_cea_sf(ULONG samplerate)
{
    switch (samplerate) {
    case 32000:
        return 1;
    case 44100:
        return 2;
    case 48000:
        return 3;
    case 88200:
        return 4;
    case 96000:
        return 5;
    case 176400:
        return 6;
    case 192000:
        return 7;
    default:
        return 0; /* refer to stream header */
    }
}

static void hdmi_write_audio_infoframe(ULONG pb, ULONG samplerate)
{
    ULONG slot_base = pb + HDMI_OFFSET + 0x400 + 4 * 0x24;
    UBYTE infoframe[14];
    UBYTE checksum;
    int i;

    /* Header */
    infoframe[0] = 0x84; /* Audio InfoFrame type */
    infoframe[1] = 0x01; /* Version 1 */
    infoframe[2] = 0x0A; /* Length = 10 */

    /* Data bytes */
    infoframe[3] = 0x00;                                      /* Checksum (computed below) */
    infoframe[4] = 0x11;                                      /* CC=1 (2ch), CT=1 (L-PCM) */
    infoframe[5] = (srate_to_cea_sf(samplerate) << 2) | 0x01; /* SF | SS=16bit */
    infoframe[6] = 0x00;                                      /* Format dependent */
    infoframe[7] = 0x00;                                      /* CA = 0 (FL/FR) */
    infoframe[8] = 0x00;                                      /* DM_INH=0, LSV=0 */
    for (i = 9; i < 14; i++)
        infoframe[i] = 0;

    /* Compute checksum: sum of all bytes must be 0 */
    checksum = 0;
    for (i = 0; i < 14; i++)
        if (i != 3)
            checksum += infoframe[i];
    infoframe[3] = (UBYTE) (0x100 - checksum);

    /*
     * Write to RAM packet memory.
     * Each RAM packet slot holds data packed as 32-bit LE words.
     * Header (3 bytes) + checksum + data bytes (10 bytes) = 14 bytes.
     * Written as ULONG words, low byte first.
     */
    for (i = 0; i < 14; i += 4) {
        ULONG word = 0;
        int j;
        for (j = 0; j < 4 && (i + j) < 14; j++)
            word |= ((ULONG) infoframe[i + j]) << (j * 8);
        wr32le(slot_base + i, word);
    }

    /* Enable the audio infoframe packet (bit 4 = packet_id 4) */
    wr32le(HDMI_RAM_PKT_CFG(pb), rd32le(HDMI_RAM_PKT_CFG(pb)) | (1 << 4));
}


/******************************************************************************
** HDMI MAI setup *************************************************************
******************************************************************************/

/*
 * Initialize the HDMI MAI for audio output.
 *
 * Based on Linux vc4_hdmi.c (BCM2835) and verified against
 * working bare-metal implementations.
 *
 * Key points:
 * - Reset sequence from Linux vc4_hdmi: RESET, ERRORF, FLUSH (separate writes)
 * - Software provides complete IEC958 subframes, including parity
 * - BIT_REVERSE | FORMAT_REVERSE required for correct serialization
 * - Channel map: 3-bit fields at bits 0-2 (ch0) and 4-6 (ch1)
 * - MAI_SMP: N = hsm_clock / samplerate, M = 0
 */
void hdmi_mai_init(struct RPiHDMIData *dd)
{
    ULONG pb = dd->periiobase;
    ULONG srate_enum = srate_to_mai_enum(dd->samplerate);
    ULONG n_value = srate_to_n(dd->samplerate);

    /*
     * Reset MAI — matches Linux vc4_hdmi_audio_reset() sequence.
     * Three separate writes: RESET, then clear ERRORF, then FLUSH.
     * This resets the internal channel counter and FIFO state.
     */
    wr32le(HDMI_MAI_CTL(pb), MAI_CTL_RESET);
    udelay(pb, 100);
    wr32le(HDMI_MAI_CTL(pb), MAI_CTL_ERRORF);
    wr32le(HDMI_MAI_CTL(pb), MAI_CTL_FLUSH);
    udelay(pb, 100);

    /* Set audio format: PCM at bits 23:16, sample rate at bits 15:8 */
    wr32le(HDMI_MAI_FMT(pb), MAI_FMT_FORMAT_PCM | MAI_FMT_RATE(srate_enum));

    /*
     * FIFO thresholds — matches Linux vc4_hdmi values (all 0x10).
     */
    wr32le(HDMI_MAI_THR(pb), MAI_THR_DREQL(0x10) | MAI_THR_DREQH(0x10) | MAI_THR_PANICL(0x10) | MAI_THR_PANICH(0x10));

    /* MAI_CONFIG: BIT_REVERSE | FORMAT_REVERSE | channel_mask=0x03 (stereo) */
    wr32le(HDMI_MAI_CONFIG(pb), MAI_CONFIG_BIT_REVERSE | MAI_CONFIG_FORMAT_REVERSE | MAI_CONFIG_CHANNEL_MASK(0x03));

    /* Channel map for stereo: 3-bit fields at bits 2:0 (ch0) and 6:4 (ch1) */
    wr32le(HDMI_MAI_CHANNEL_MAP(pb), 0x08);

    /*
     * Audio packet config (matches Linux):
     *   B_FRAME_IDENTIFIER = 0x8 at bits [13:10]
     *   CEA channel mask = 0x03 (channels 0 and 1 active)
     *   ZERO_DATA_ON_INACTIVE_CHANNELS
     *   ZERO_DATA_ON_SAMPLE_FLAT
     */
    wr32le(HDMI_AUDIO_PKT_CFG(pb),
           AUDIO_PKT_ZERO_DATA_ON_FLAT | AUDIO_PKT_ZERO_DATA_ON_INACTIVE | AUDIO_PKT_B_FRAME_ID(0x8) |
               AUDIO_PKT_CEA_MASK(0x03));

    /*
     * Sample rate clock divider.
     * MAI_SMP register: bits 31:8 = N (numerator), bits 7:0 = M (denominator-1).
     * Clock ratio = N / (M+1) ≈ hsm_clock / samplerate.
     */
    wr32le(HDMI_MAI_SMP(pb), (216000000 / dd->samplerate) << 8);

    /*
     * CTS/N audio clock recovery.
     * N is set from the HDMI spec standard values.
     * CTS is set to external mode — the hardware derives CTS from
     * the pixel clock automatically.
     * Linux also writes CTS_0 and CTS_1 explicitly.
     * CTS = (pixel_clock * N) / (128 * samplerate)
     * RPi 3B+ default pixel clock ≈ 148500 kHz (1080p60) or 74250 kHz (720p60).
     * We read CTS_0 first to get the hardware-derived value, then write it back.
     */
    wr32le(HDMI_CRP_CFG(pb), CRP_CFG_EXTERNAL_CTS_EN | CRP_CFG_N(n_value));

    {
        ULONG cts = rd32le(HDMI_CTS_0(pb));
        if (cts == 0) {
            /* Fallback: compute CTS assuming 148500 kHz pixel clock */
            cts = (148500UL * n_value) / (128 * (dd->samplerate / 1000));
        }
        wr32le(HDMI_CTS_0(pb), cts);
        wr32le(HDMI_CTS_1(pb), cts);
    }

    /* Write Audio InfoFrame to RAM packet memory */
    hdmi_write_audio_infoframe(pb, dd->samplerate);

    /*
     * Enable MAI (matches Linux vc4_hdmi_audio_prepare MAI_CTL write).
     * WHOLSMP + CHALIGN: L/R pairs consumed atomically.
     * Parity is already encoded in software, so leave PAREN cleared.
     * Note: Linux does NOT set DLATE/ERRORE/ERRORF at enable time.
     */
    wr32le(HDMI_MAI_CTL(pb), MAI_CTL_CHALIGN | MAI_CTL_WHOLSMP | MAI_CTL_CHNUM(2) | MAI_CTL_ENABLE);

    /* Initialize IEC958 channel status for this sample rate (separate L/R) */
    spdif_setup_channel_status(dd->channel_status_l, dd->channel_status_r, dd->samplerate);
    dd->frame_counter = 0;
}

/*
 * Stop the HDMI MAI audio output.
 */
void hdmi_mai_stop(struct RPiHDMIData *dd)
{
    ULONG pb = dd->periiobase;

    wr32le(HDMI_MAI_CTL(pb), MAI_CTL_FLUSH | MAI_CTL_DLATE | MAI_CTL_ERRORE | MAI_CTL_ERRORF);

    udelay(pb, 100);
}


/******************************************************************************
** DMA setup ******************************************************************
******************************************************************************/

void dma_build_control_blocks(struct RPiHDMIData *dd, ULONG peribase)
{
    int i;

    for (i = 0; i < 2; i++) {
        struct DMAControlBlock *cb = dd->cb[i];

        cb->ti = DMA_TI_INTEN | DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ | DMA_TI_SRC_INC | DMA_TI_BURST_LENGTH(2) |
                 DMA_TI_PERMAP(DMA_DREQ_HDMI) | DMA_TI_NO_WIDE_BURSTS;

        cb->source_ad = GPU_BUS_ADDR(dd->dmabuf[i]);
        cb->dest_ad = HDMI_MAI_DATA_BUS;
        cb->txfr_len = dd->dmabuf_size;
        cb->stride = 0;
        cb->nextconbk = GPU_BUS_ADDR(dd->cb[1 - i]);
        cb->reserved[0] = 0;
        cb->reserved[1] = 0;
    }
}

void dma_setup(ULONG peribase, ULONG channel, ULONG cb_bus_addr)
{
    ULONG dma_base = peribase + 0x007000 + channel * 0x100;
    ULONG enable_addr = peribase + 0x007FF0;

    wr32le(enable_addr, rd32le(enable_addr) | (1 << channel));
    wr32le(dma_base + 0x00, DMA_CS_RESET);
    udelay(peribase, 10);
    wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END);
    wr32le(dma_base + 0x04, cb_bus_addr);
    wr32le(dma_base + 0x00, DMA_CS_WAIT_FOR_WRITES | DMA_CS_PANIC_PRI(15) | DMA_CS_PRI(8) | DMA_CS_ACTIVE);
    udelay(peribase, 10);
}

void dma_stop(ULONG peribase, ULONG channel)
{
    ULONG dma_base = peribase + 0x007000 + channel * 0x100;

    wr32le(dma_base + 0x00, 0);
    udelay(peribase, 50);
    wr32le(dma_base + 0x00, DMA_CS_RESET);
    udelay(peribase, 100);
    wr32le(dma_base + 0x04, 0);
    wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END);
}
