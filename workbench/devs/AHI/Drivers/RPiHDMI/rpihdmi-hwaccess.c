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
 *  Register offsets are for the BCM2835 HD/HDMI core register blocks.
 *  No VCHIQ firmware interaction needed — HDMI link is already
 *  established by the VideoCore firmware at boot.
 */

#include <config.h>

#include <exec/types.h>
#include <aros/macros.h>

#define DEBUG 0
#include <aros/debug.h>

#include "DriverData.h"
#include "rpihdmi-hwaccess.h"
#include "rpihdmi-iec958.h"

#define RPIHDMI_CHANNEL_MASK 0x03

/*
 * Microsecond delay using a busy loop on the system timer.
 */
static void udelay(IPTR peribase, ULONG us)
{
    volatile ULONG *clo = (volatile ULONG *) (peribase + 0x003004);
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

static void hdmi_write_audio_infoframe(struct RPiHDMIData *dd)
{
    ULONG slot_base = HDMI_RAM_PKT_START(dd) + 4 * 0x24;
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
    infoframe[5] = (srate_to_cea_sf(dd->samplerate) << 2) | 0x01; /* SF | SS=16bit */
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
    wr32le(HDMI_RAM_PKT_CFG(dd), rd32le(HDMI_RAM_PKT_CFG(dd)) | (1 << 4));
}


/******************************************************************************
** HDMI MAI setup *************************************************************
******************************************************************************/

/*
 * Initialize the HDMI MAI for audio output.
 *
 * Programming sequence verified against working bare-metal
 * implementations.
 *
 * Key points:
 * - Reset sequence: RESET, ERRORF, FLUSH (separate writes)
 * - Software provides complete IEC958 subframes, including parity
 * - BIT_REVERSE | FORMAT_REVERSE required for correct serialization
 * - Channel map: 3-bit fields at bits 0-2 (ch0) and 4-6 (ch1)
 * - MAI_SMP: N = hsm_clock / samplerate, M = 0
 */
void hdmi_mai_init(struct RPiHDMIData *dd)
{
    struct DriverBase *AHIsubBase =
        (struct DriverBase *) dd->ahisubbase;

    ULONG pb = dd->periiobase;
    ULONG srate_enum = srate_to_mai_enum(dd->samplerate);
    ULONG n_value = srate_to_n(dd->samplerate);

    /*
     * Reset MAI.
     * Three separate writes: RESET, then clear ERRORF, then FLUSH.
     * This resets the internal channel counter and FIFO state.
     */
    wr32le(HDMI_MAI_CTL(dd), MAI_CTL_RESET);
    udelay(pb, 100);
    wr32le(HDMI_MAI_CTL(dd), MAI_CTL_ERRORF);
    wr32le(HDMI_MAI_CTL(dd), MAI_CTL_FLUSH);
    udelay(pb, 100);

    /* Set audio format: PCM at bits 23:16, sample rate at bits 15:8 */
    wr32le(HDMI_MAI_FMT(dd), MAI_FMT_FORMAT_PCM | MAI_FMT_RATE(srate_enum));

    /*
     * FIFO thresholds.
     */
    ULONG dreq_threshold = dd->soc->mai_dreq_threshold;
    ULONG panic_threshold = dd->soc->mai_panic_threshold;

    wr32le(HDMI_MAI_THR(dd), MAI_THR_DREQL(dreq_threshold) | MAI_THR_DREQH(dreq_threshold) | MAI_THR_PANICL(panic_threshold) | MAI_THR_PANICH(panic_threshold));

    /* MAI_CONFIG: temporarily test one MAI channel at a time. */
    wr32le(HDMI_MAI_CONFIG(dd),
           MAI_CONFIG_BIT_REVERSE |
           MAI_CONFIG_FORMAT_REVERSE |
           MAI_CONFIG_CHANNEL_MASK(RPIHDMI_CHANNEL_MASK));

    /* Channel map for stereo: bcm283x :3-bit fields at bits 2:0 (ch0) and 6:4 (ch1)
     *                         bcm2711 :4-bit fields
     */
    wr32le(HDMI_MAI_CHANNEL_MAP(dd), dd->soc->hdmi_mai_channel_map);

    /*
     * Audio packet config:
     *   B_FRAME_IDENTIFIER = 0x8 at bits [13:10]
     *   CEA channel mask = 0x03 (channels 0 and 1 active)
     *   ZERO_DATA_ON_INACTIVE_CHANNELS
     *   ZERO_DATA_ON_SAMPLE_FLAT
     */
    wr32le(HDMI_AUDIO_PKT_CFG(dd),
           AUDIO_PKT_ZERO_DATA_ON_FLAT | AUDIO_PKT_ZERO_DATA_ON_INACTIVE | AUDIO_PKT_B_FRAME_ID(0x8) |
               AUDIO_PKT_CEA_MASK(0x03));

    D(bug("[RPiHDMI] channel: MAP=%08lx CONFIG=%08lx AUDIO=%08lx\n",
        rd32le(HDMI_MAI_CHANNEL_MAP(dd)),
        rd32le(HDMI_MAI_CONFIG(dd)),
        rd32le(HDMI_AUDIO_PKT_CFG(dd))));
    /*
     * Sample rate clock divider.
     * MAI_SMP register: bits 31:8 = N (numerator), bits 7:0 = M (denominator-1).
     * Clock ratio = N / (M+1) = hsm_clock / samplerate. Round N (rather than
     * truncate) to halve the residual rate error.
     */
    wr32le(HDMI_MAI_SMP(dd),
           ((dd->soc->hsm_clock + dd->samplerate / 2) / dd->samplerate) << 8);

    /*
     * CTS/N audio clock recovery.
     * N is set from the HDMI spec standard values.
     * CTS is set to external mode — the hardware derives CTS from
     * the pixel clock automatically.
     * CTS = (pixel_clock * N) / (128 * samplerate)
     * RPi 3B+ default pixel clock ≈ 148500 kHz (1080p60) or 74250 kHz (720p60).
     * We read CTS_0 first to get the hardware-derived value, then write it back.
     */
    wr32le(HDMI_CRP_CFG(dd), CRP_CFG_EXTERNAL_CTS_EN | CRP_CFG_N(n_value));

    {
        ULONG cts = rd32le(HDMI_CTS_0(dd));
        if (cts == 0) {
            /* Fallback: compute CTS assuming 148500 kHz pixel clock */
            cts = (148500UL * n_value) / (128 * (dd->samplerate / 1000));
        }
        wr32le(HDMI_CTS_0(dd), cts);
        wr32le(HDMI_CTS_1(dd), cts);
    }

    /* Write Audio InfoFrame to RAM packet memory */
    hdmi_write_audio_infoframe(dd);

    /*
     * Enable MAI.
     * WHOLSMP + CHALIGN: L/R pairs consumed atomically.
     * Parity is already encoded in software, so leave PAREN cleared.
     * Note: DLATE/ERRORE/ERRORF are deliberately left cleared at
     * enable time.
     */
    wr32le(HDMI_MAI_CTL(dd), MAI_CTL_CHALIGN | MAI_CTL_WHOLSMP | MAI_CTL_CHNUM(2) | MAI_CTL_ENABLE);

    /* Initialize IEC958 channel status for this sample rate (separate L/R) */
    spdif_setup_channel_status(dd->channel_status_l, dd->channel_status_r, dd->samplerate);
    dd->frame_counter = 0;
}

/*
 * Stop the HDMI MAI audio output.
 */
void hdmi_mai_stop(struct RPiHDMIData *dd)
{
    wr32le(HDMI_MAI_CTL(dd), MAI_CTL_FLUSH | MAI_CTL_DLATE | MAI_CTL_ERRORE | MAI_CTL_ERRORF);

    udelay(dd->periiobase, 100);
}
