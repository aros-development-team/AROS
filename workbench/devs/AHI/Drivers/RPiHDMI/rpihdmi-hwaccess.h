#ifndef AHI_Drivers_RPiHDMI_hwaccess_h
#define AHI_Drivers_RPiHDMI_hwaccess_h

#include <exec/types.h>
#include <aros/macros.h>

#include "DriverData.h"

/*
 * GPU bus address for uncached DMA access.
 * On BCM2835/2836, ARM physical 0x00000000 maps to GPU bus 0xC0000000
 * (uncached alias).
 */
#define GPU_BUS_ADDR(x) BCM2708_DMA_BUS_ADDR(x)

/* Register access helpers (little-endian, with ARM memory barriers) */
static inline void __dsb(void)
{
    asm volatile("dsb sy" ::: "memory");
}
static inline void __dmb(void)
{
    asm volatile("dmb sy" ::: "memory");
}

static inline ULONG rd32le(ULONG addr)
{
    ULONG val;
    __dmb();
    val = AROS_LE2LONG(*(volatile ULONG *) addr);
    __dsb();
    return val;
}

static inline void wr32le(ULONG addr, ULONG val)
{
    __dsb();
    *(volatile ULONG *) addr = AROS_LONG2LE(val);
    __dmb();
}

/*
 * HDMI HD register block (MAI control)
 */
#define HDMI_MAI_CTL(dd) \
    ((dd->periiobase) + (dd->soc->mai_base) + 0x14)
#define HDMI_MAI_THR(dd) \
    ((dd->periiobase) + (dd->soc->mai_base) + 0x18)
#define HDMI_MAI_FMT(dd) \
    ((dd->periiobase) + (dd->soc->mai_base) + 0x1C)
#define HDMI_MAI_DATA(dd) \
    ((dd->periiobase) + (dd->soc->mai_base) + 0x20)
#define HDMI_MAI_SMP(dd) \
    ((dd->periiobase) + (dd->soc->mai_base) + 0x2c)

/*
 * HDMI register block
 */
#define HDMI_MAI_CHANNEL_MAP(dd) \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x090)
#define HDMI_MAI_CONFIG(dd)      \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x094)
#define HDMI_AUDIO_PKT_CFG(dd)   \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x09C)
#define HDMI_RAM_PKT_CFG(dd)     \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x0A0)
#define HDMI_RAM_PKT_STATUS(dd)  \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x0A4)
#define HDMI_CRP_CFG(dd)         \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x0A8)
#define HDMI_CTS_0(dd)           \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x0AC)
#define HDMI_CTS_1(dd)           \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x0B0)
#define HDMI_SCHEDULER_CONTROL(dd) \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x0C0)
#define HDMI_RAM_PKT_START(dd)   \
    ((dd->periiobase) + (dd->soc->hdmi_base) + 0x400)

/* MAI_CTL bits */
#define MAI_CTL_RESET    (1 << 0)
#define MAI_CTL_ERRORF   (1 << 1)
#define MAI_CTL_ERRORE   (1 << 2)
#define MAI_CTL_ENABLE   (1 << 3)
#define MAI_CTL_CHNUM(x) (((x) & 0xF) << 4)
#define MAI_CTL_PAREN    (1 << 8)
#define MAI_CTL_FLUSH    (1 << 9)
#define MAI_CTL_WHOLSMP  (1 << 12)
#define MAI_CTL_CHALIGN  (1 << 13)
#define MAI_CTL_DLATE    (1 << 15)

/* MAI_THR: DREQ and panic thresholds */
#define MAI_THR_DREQL(x)  (((x) & 0x3F) << 0)
#define MAI_THR_DREQH(x)  (((x) & 0x3F) << 8)
#define MAI_THR_PANICL(x) (((x) & 0x3F) << 16)
#define MAI_THR_PANICH(x) (((x) & 0x3F) << 24)

/* MAI_FMT: sample rate and audio format */
#define MAI_FMT_RATE(x)    (((x) & 0xFF) << 8)
#define MAI_FMT_FORMAT(x)  (((x) & 0xFF) << 16)
#define MAI_FMT_FORMAT_PCM MAI_FMT_FORMAT(2)

/* MAI_CONFIG bits (HDMI block) */
#define MAI_CONFIG_BIT_REVERSE     (1 << 26)
#define MAI_CONFIG_FORMAT_REVERSE  (1 << 27)
#define MAI_CONFIG_CHANNEL_MASK(x) ((x) & 0xFFFF)

/* Audio packet config bits (HDMI block) */
#define AUDIO_PKT_CEA_MASK(x)           ((x) & 0xFF)
#define AUDIO_PKT_B_FRAME_ID(x)         (((x) & 0xF) << 10)
#define AUDIO_PKT_ZERO_DATA_ON_INACTIVE (1 << 24)
#define AUDIO_PKT_ZERO_DATA_ON_FLAT     (1 << 29)

/* CRP_CFG bits */
#define CRP_CFG_EXTERNAL_CTS_EN (1 << 24)
#define CRP_CFG_N(x)            ((x) & 0xFFFFF)


/* Bus address of MAI DATA register (DMA destination) */
#define HDMI_MAI_DATA_BUS 0x7E808020

/*
 * BCM2835 DMA IRQ numbers.
 * DMA channel N uses GPU IRQ (16 + N).
 */
#define BCM_IRQ_DMA0 16

/* Sample rate enum values for MAI_FMT */
#define SRATE_8000   1
#define SRATE_11025  2
#define SRATE_12000  3
#define SRATE_16000  4
#define SRATE_22050  5
#define SRATE_24000  6
#define SRATE_32000  7
#define SRATE_44100  8
#define SRATE_48000  9
#define SRATE_88200  10
#define SRATE_96000  12
#define SRATE_176400 14
#define SRATE_192000 15

/* Hardware setup/teardown functions */
void hdmi_mai_init(struct RPiHDMIData *dd);
void hdmi_mai_stop(struct RPiHDMIData *dd);

#endif /* AHI_Drivers_RPiHDMI_hwaccess_h */
