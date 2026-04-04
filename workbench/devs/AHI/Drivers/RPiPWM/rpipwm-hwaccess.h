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
#define GPU_BUS_ADDR(x) (0xC0000000 | (ULONG) (x))

/* Register access helpers (little-endian, with ARM memory barriers) */
static inline void __dsb(void)
{
    asm volatile("dsb" ::: "memory");
}
static inline void __dmb(void)
{
    asm volatile("dmb" ::: "memory");
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

/* PWM range: 10-bit (1024 levels) gives decent audio quality */
#define PWM_AUDIO_RANGE 1024

/* DMA channel to use for audio (channel 5, avoiding GPU-reserved 0-3) */
#define PWM_DMA_CHANNEL 5

/* PLLD clock frequency (500 MHz on BCM2835/2836) */
#define PLLD_FREQ 500000000

/*
 * BCM2835 DMA IRQ numbers.
 * DMA channel N uses GPU IRQ (16 + N).
 * These match the IRQ_DMA0..IRQ_DMA12 defines in bcm2708.h.
 */
#define BCM_IRQ_DMA0 16

/* DMA control block TI bits (local defines for self-contained build) */
#define DMA_TI_INTEN          (1 << 0)
#define DMA_TI_WAIT_RESP      (1 << 3)
#define DMA_TI_DEST_DREQ      (1 << 6)
#define DMA_TI_SRC_INC        (1 << 8)
#define DMA_TI_PERMAP(x)      (((x) & 0x1F) << 16)
#define DMA_TI_NO_WIDE_BURSTS (1 << 26)

/* DMA CS bits */
#define DMA_CS_ACTIVE          (1 << 0)
#define DMA_CS_END             (1 << 1)
#define DMA_CS_INT             (1 << 2)
#define DMA_CS_WAIT_FOR_WRITES (1 << 28)
#define DMA_CS_PANIC_PRI(x)    (((x) & 0xF) << 20)
#define DMA_CS_PRI(x)          (((x) & 0xF) << 16)
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

/* Clock Manager bits */
#define CM_PASSWORD 0x5A000000
#define CM_SRC_PLLD 6
#define CM_BUSY     (1 << 7)
#define CM_ENAB     (1 << 4)
#define CM_MASH(x)  (((x) & 3) << 9)

/* DMA DREQ peripheral map IDs */
#define DMA_DREQ_PWM 5

/* Bus address of PWM FIFO register (DMA destination) */
#define PWM_FIF1_BUS 0x7E20C018

/* Hardware setup/teardown functions */
void pwm_gpio_setup(ULONG peribase);
void pwm_gpio_restore(ULONG peribase);
void pwm_clock_setup(ULONG peribase, ULONG samplerate, ULONG range);
void pwm_clock_stop(ULONG peribase);
void pwm_init(ULONG peribase, ULONG range);
void pwm_stop(ULONG peribase);
void dma_setup(ULONG peribase, ULONG channel, ULONG cb_bus_addr);
void dma_stop(ULONG peribase, ULONG channel);

/* Build DMA control blocks for double-buffered PWM playback */
void dma_build_control_blocks(struct RPiPWMData *dd, ULONG peribase);

/* DMA interrupt handler (called from KrnAddIRQHandler) */
void dma_irq_handler(struct RPiPWMData *data, void *data2);

#endif /* AHI_Drivers_RPiPWM_hwaccess_h */
