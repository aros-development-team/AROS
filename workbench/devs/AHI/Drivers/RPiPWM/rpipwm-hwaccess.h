#ifndef AHI_Drivers_RPiPWM_hwaccess_h
#define AHI_Drivers_RPiPWM_hwaccess_h

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

static inline ULONG rd32le(IPTR addr)
{
    ULONG val;
    __dmb();
    val = AROS_LE2LONG(*(volatile ULONG *) addr);
    __dsb();
    return val;
}

static inline void wr32le(IPTR addr, ULONG val)
{
    __dsb();
    *(volatile ULONG *) addr = AROS_LONG2LE(val);
    __dmb();
}

/* PWM range: 10-bit (1024 levels) gives decent audio quality */
#define PWM_AUDIO_RANGE 1024

/* The audio DMA channel is allocated at runtime from dma.resource. */

/*
 * The PLL the clock manager divides down for the PWM: 500 MHz on the
 * BCM283x, 750 MHz on the BCM2711. The wrong one runs the PWM half again too
 * fast and rejects a firmware clock that was in fact correct.
 */
#define PLLD_FREQ         500000000
#define PLLD_FREQ_BCM2711 750000000

/* DMA control block TI bits (local defines for self-contained build) */
#define DMA_TI_INTEN          (1 << 0)
#define DMA_TI_WAIT_RESP      (1 << 3)
#define DMA_TI_DEST_DREQ      (1 << 6)
#define DMA_TI_SRC_INC        (1 << 8)
#define DMA_TI_PERMAP(x)      (((x) & 0x1F) << 16)
#define DMA_TI_NO_WIDE_BURSTS (1 << 26)

/* DMA CS bits */
/* Run state and acknowledge live in bcm2708_dma.h, shared with the other
 * DMA-driven AHI driver - they have to agree. */
#define DMA_CS_ACTIVE          (1 << 0)
#define DMA_CS_END             (1 << 1)
#define DMA_CS_INT             (1 << 2)
#define DMA_CS_ABORT           (1 << 30)
#define DMA_CS_RESET           (1 << 31)

/* PWM CTL bits */
#define PWM_CTL_PWEN1 (1 << 0)
#define PWM_CTL_MSEN1 (1 << 7)
#define PWM_CTL_USEF1 (1 << 5)
#define PWM_CTL_CLRF1 (1 << 6)
#define PWM_CTL_PWEN2 (1 << 8)
#define PWM_CTL_MSEN2 (1 << 15)
#define PWM_CTL_USEF2 (1 << 13)

/* PWM DMAC bits */
#define PWM_DMAC_ENAB     (1 << 31)
#define PWM_DMAC_PANIC(x) (((x) & 0xFF) << 8)
#define PWM_DMAC_DREQ(x)  (((x) & 0xFF) << 0)

/* Clock Manager PWM clock registers */
#define CM_PWMCTL_ADDR(peribase) ((peribase) + 0x1010A0)
#define CM_PWMDIV_ADDR(peribase) ((peribase) + 0x1010A4)

/* Clock Manager bits */
#define CM_PASSWORD 0x5A000000
#define CM_SRC_PLLD 6
#define CM_BUSY     (1 << 7)
#define CM_ENAB     (1 << 4)
#define CM_MASH(x)  (((x) & 3) << 9)

/*
 * DREQ peripheral map IDs. 5 paces the first PWM block everywhere. The
 * BCM2711's second block - the one GPIO 40/41 reach - is paced by 1 (DSI on
 * the BCM283x); no device tree names it, so it was measured on a Pi 4.
 */
#define DMA_DREQ_PWM        5
#define DMA_DREQ_PWM_2711   1

/*
 * Mirrored from hardware/bcm2708.h, which cannot be included here: it derives
 * every address from an ARM_PERIIOBASE the caller must define, and this driver
 * only learns its peripheral base at run time.
 */
#define BCM2711_PERIIOBASE      0xFE000000

/*
 * Which PWM block reaches the headphone jack. The BCM283x has one; on the
 * BCM2711 ALT0 on GPIO 40/41 selects the *second* one (its tree names the
 * groups pwm1-0-gpio40/pwm1-1-gpio41 and has no pwm0 group for either pin).
 * Same mux value, different peripheral - block 0 there consumes samples
 * happily and outputs on pins the board does not route.
 */
#define PWM_BLOCK_OFFSET        0x20C000
#define PWM_BLOCK_OFFSET_2711   0x20C800

/* Bus address of the PWM FIFO register (the DMA destination) */
#define PWM_FIF1_BUS            0x7E20C018
#define PWM_FIF1_BUS_2711       0x7E20C818

static inline IPTR pwm_block(IPTR peribase)
{
    return peribase + ((peribase == BCM2711_PERIIOBASE)
                          ? PWM_BLOCK_OFFSET_2711 : PWM_BLOCK_OFFSET);
}

static inline ULONG pwm_fifo_bus(IPTR peribase)
{
    return (peribase == BCM2711_PERIIOBASE) ? PWM_FIF1_BUS_2711 : PWM_FIF1_BUS;
}

static inline ULONG pwm_dreq(IPTR peribase)
{
    return (peribase == BCM2711_PERIIOBASE) ? DMA_DREQ_PWM_2711 : DMA_DREQ_PWM;
}

/* Hardware setup/teardown functions */
BOOL pwm_audio_present(struct DriverBase *AHIsubBase);
void pwm_gpio_setup(struct DriverBase *AHIsubBase, IPTR peribase);
void pwm_clock_setup(struct DriverBase *AHIsubBase, IPTR peribase, ULONG samplerate, ULONG range);
void pwm_clock_stop(struct DriverBase *AHIsubBase, IPTR peribase);
void pwm_init(IPTR peribase, ULONG range);
void pwm_rearm(IPTR peribase, ULONG range);
void pwm_stop(IPTR peribase);
void pwm_ramp_dc(IPTR peribase, ULONG from, ULONG to);
void pwm_fifo_enable(IPTR peribase);
void pwm_dat_hold(IPTR peribase, ULONG value);
ULONG dma_current_level(struct RPiPWMData *dd);
void dma_setup(IPTR peribase, ULONG channel, ULONG cb_bus_addr);
void dma_stop(IPTR peribase, ULONG channel);

/* Build DMA control blocks for double-buffered PWM playback */
void dma_build_control_blocks(struct RPiPWMData *dd, IPTR peribase);

#if 0
/* Measures which DREQ paces this SoC's PWM FIFO - how DMA_DREQ_PWM_2711 was
 * found. Clobbers cb[0]; read the body before re-enabling. */
ULONG dma_probe_dreq(struct DriverBase *AHIsubBase, struct RPiPWMData *dd, ULONG expect);
#endif

/* DMA interrupt handler (called from KrnAddIRQHandler) */
void dma_irq_handler(struct RPiPWMData *data, void *data2);

#endif /* AHI_Drivers_RPiPWM_hwaccess_h */
