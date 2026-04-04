/*
 *  BCM2835 PWM Audio hardware access for Raspberry Pi
 *
 *  Configures GPIO pins 40/45 for PWM output, sets up the clock manager
 *  for the desired sample rate, initializes the PWM peripheral in FIFO
 *  mode with DMA, and manages DMA control blocks for double-buffered
 *  audio playback.
 */

#include <config.h>

#include <exec/types.h>
#include <aros/macros.h>

#include "rpipwm-hwaccess.h"

/*
 * Microsecond delay using a busy loop on the system timer.
 * This is used during clock/PWM setup where we need short delays
 * but cannot use Delay() (which needs DOS and is too coarse).
 */
static void udelay(ULONG peribase, ULONG us)
{
    volatile ULONG *clo = (volatile ULONG *) (ULONG) (peribase + 0x003004);
    ULONG start = AROS_LE2LONG(*clo);

    while ((AROS_LE2LONG(*clo) - start) < us)
        ;
}

/******************************************************************************
** GPIO setup for PWM audio ***************************************************
******************************************************************************/

/*
 * Configure GPIO pins 40 and 45 to ALT0 function (PWM0 and PWM1).
 *
 * Pin 40: GPFSEL4, bits 2:0 = ALT0 (0b100) = PWM0_OUT (left channel)
 * Pin 45: GPFSEL4, bits 17:15 = ALT0 (0b100) = PWM1_OUT (right channel)
 *
 * These pins are routed to the 3.5mm headphone jack on Pi 3B via a
 * low-pass RC filter on the board.
 */
void pwm_gpio_setup(ULONG peribase)
{
    ULONG gpfsel4_addr = peribase + 0x200010; /* GPFSEL4 */
    ULONG val;

    val = rd32le(gpfsel4_addr);

    /* Pin 40: bits [2:0] in GPFSEL4 (pin 40 = GPFSEL4 pin 0) */
    val &= ~(7 << 0); /* Clear bits 2:0 */
    val |= (4 << 0);  /* ALT0 = 0b100 */

    /* Pin 45: bits [17:15] in GPFSEL4 (pin 45 = GPFSEL4 pin 5) */
    val &= ~(7 << 15); /* Clear bits 17:15 */
    val |= (4 << 15);  /* ALT0 = 0b100 */

    wr32le(gpfsel4_addr, val);
}

/*
 * Restore GPIO pins 40 and 45 to input (default safe state).
 */
void pwm_gpio_restore(ULONG peribase)
{
    ULONG gpfsel4_addr = peribase + 0x200010; /* GPFSEL4 */
    ULONG val;

    val = rd32le(gpfsel4_addr);
    val &= ~(7 << 0);  /* Pin 40 → input */
    val &= ~(7 << 15); /* Pin 45 → input */
    wr32le(gpfsel4_addr, val);
}

/******************************************************************************
** Clock Manager setup ********************************************************
******************************************************************************/

/*
 * Configure the PWM clock from PLLD (500 MHz) for the desired sample rate.
 *
 * The PWM runs at: clock_freq / range samples per second.
 * So we need: clock_freq = samplerate * range
 * Divisor (12.12 fixed point) = PLLD_FREQ / (samplerate * range)
 *
 * We use MASH=1 (1-stage MASH noise-shaping) for fractional division,
 * which gives better audio quality than integer-only division.
 */
void pwm_clock_setup(ULONG peribase, ULONG samplerate, ULONG range)
{
    ULONG cm_ctl_addr = peribase + 0x1010A0; /* CM_PWMCTL */
    ULONG cm_div_addr = peribase + 0x1010A4; /* CM_PWMDIV */
    ULONG target_freq;
    ULONG divi, divf;

    /* Stop the clock */
    wr32le(cm_ctl_addr, CM_PASSWORD | (rd32le(cm_ctl_addr) & ~CM_ENAB));

    /* Wait for clock to stop (BUSY flag clears) */
    while (rd32le(cm_ctl_addr) & CM_BUSY)
        udelay(peribase, 1);

    /* Calculate divisor: 12-bit integer, 12-bit fraction */
    target_freq = samplerate * range;
    divi = PLLD_FREQ / target_freq;
    divf = ((ULONG) ((((unsigned long long) (PLLD_FREQ % target_freq)) << 12) / target_freq)) & 0xFFF;

    /* Set divisor */
    wr32le(cm_div_addr, CM_PASSWORD | (divi << 12) | divf);

    /* Start clock: source = PLLD (6), MASH = 1 */
    wr32le(cm_ctl_addr, CM_PASSWORD | CM_MASH(1) | CM_SRC_PLLD);

    udelay(peribase, 10);

    /* Enable clock */
    wr32le(cm_ctl_addr, CM_PASSWORD | CM_MASH(1) | CM_SRC_PLLD | CM_ENAB);

    /* Wait for clock to start */
    while (!(rd32le(cm_ctl_addr) & CM_BUSY))
        udelay(peribase, 1);
}

/*
 * Stop the PWM clock.
 */
void pwm_clock_stop(ULONG peribase)
{
    ULONG cm_ctl_addr = peribase + 0x1010A0;

    wr32le(cm_ctl_addr, CM_PASSWORD | (rd32le(cm_ctl_addr) & ~CM_ENAB));

    while (rd32le(cm_ctl_addr) & CM_BUSY)
        udelay(peribase, 1);
}

/******************************************************************************
** PWM peripheral setup *******************************************************
******************************************************************************/

/*
 * Initialize both PWM channels for audio output.
 *
 * Both channels are configured in Mark/Space mode with FIFO input.
 * The DMA writes interleaved L/R samples to the single FIFO register.
 * PWM channel 1 reads even words (left), channel 2 reads odd words (right).
 *
 * The channels are enabled (PWEN) here because the PWM DREQ signal
 * only asserts when channels are active. DMA must be started promptly
 * after this call to feed the FIFO before it underruns.
 */
void pwm_init(ULONG peribase, ULONG range)
{
    ULONG pwm_base = peribase + 0x20C000;

    /* Disable PWM */
    wr32le(pwm_base + 0x00, 0); /* PWM_CTL = 0 */

    udelay(peribase, 10);

    /* Set range for both channels */
    wr32le(pwm_base + 0x10, range); /* PWM_RNG1 */
    wr32le(pwm_base + 0x20, range); /* PWM_RNG2 */

    /* Clear FIFO */
    wr32le(pwm_base + 0x00, PWM_CTL_CLRF1);

    udelay(peribase, 10);

    /* Clear status flags */
    wr32le(pwm_base + 0x04, 0xFFFFFFFF); /* PWM_STA: write 1 to clear */

    /* Configure DMA:
     * ENAB=1, PANIC threshold=7, DREQ threshold=3
     * DREQ threshold determines when the FIFO requests more data.
     * A low threshold keeps the FIFO fed without underruns.
     */
    wr32le(pwm_base + 0x08, PWM_DMAC_ENAB | PWM_DMAC_PANIC(7) | PWM_DMAC_DREQ(3));

    /* Enable both channels:
     * USEF1/2 = use FIFO (not data register)
     * PWEN1/2 = enable channel
     * No MODE bit = PWM mode (not serializer) for analog output
     * MSEN1/2 = Mark/Space mode for better audio quality
     */
    wr32le(pwm_base + 0x00,
           PWM_CTL_USEF1 | PWM_CTL_PWEN1 | PWM_CTL_MSEN1 | PWM_CTL_USEF2 | PWM_CTL_PWEN2 | PWM_CTL_MSEN2);
}

/*
 * Stop the PWM peripheral.
 */
void pwm_stop(ULONG peribase)
{
    ULONG pwm_base = peribase + 0x20C000;

    /* Disable PWM */
    wr32le(pwm_base + 0x00, 0);

    /* Disable DMA */
    wr32le(pwm_base + 0x08, 0);

    /* Clear FIFO and status */
    wr32le(pwm_base + 0x00, PWM_CTL_CLRF1);
    wr32le(pwm_base + 0x04, 0xFFFFFFFF);
}

/******************************************************************************
** DMA setup ******************************************************************
******************************************************************************/

/*
 * Build the two DMA control blocks for double-buffered playback.
 * CB[0] plays dmabuf[0] then chains to CB[1].
 * CB[1] plays dmabuf[1] then chains to CB[0].
 * Each CB generates an interrupt on completion so the slave task
 * knows to refill the consumed buffer.
 */
void dma_build_control_blocks(struct RPiPWMData *dd, ULONG peribase)
{
    int i;

    for (i = 0; i < 2; i++) {
        struct DMAControlBlock *cb = dd->cb[i];

        cb->ti = DMA_TI_INTEN | DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ | DMA_TI_SRC_INC | DMA_TI_PERMAP(DMA_DREQ_PWM) |
                 DMA_TI_NO_WIDE_BURSTS;

        cb->source_ad = GPU_BUS_ADDR(dd->dmabuf[i]);
        cb->dest_ad = PWM_FIF1_BUS;
        cb->txfr_len = dd->dmabuf_size;
        cb->stride = 0;
        /* Chain to the other CB */
        cb->nextconbk = GPU_BUS_ADDR(dd->cb[1 - i]);
        cb->reserved[0] = 0;
        cb->reserved[1] = 0;
    }
}

/*
 * Start DMA on the specified channel.
 */
void dma_setup(ULONG peribase, ULONG channel, ULONG cb_bus_addr)
{
    ULONG dma_base = peribase + 0x007000 + channel * 0x100;
    ULONG enable_addr = peribase + 0x007FF0;

    /* Enable the DMA channel */
    wr32le(enable_addr, rd32le(enable_addr) | (1 << channel));

    /* Reset the channel */
    wr32le(dma_base + 0x00, DMA_CS_RESET);

    udelay(peribase, 10);

    /* Clear status bits */
    wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END);

    /* Set control block address */
    wr32le(dma_base + 0x04, cb_bus_addr);

    /* Start DMA: active, priority 8, panic priority 15, wait for writes */
    wr32le(dma_base + 0x00, DMA_CS_WAIT_FOR_WRITES | DMA_CS_PANIC_PRI(15) | DMA_CS_PRI(8) | DMA_CS_ACTIVE);
}

/*
 * Stop DMA on the specified channel.
 */
void dma_stop(ULONG peribase, ULONG channel)
{
    ULONG dma_base = peribase + 0x007000 + channel * 0x100;

    /* Deactivate first, then reset */
    wr32le(dma_base + 0x00, 0); /* Clear ACTIVE */
    udelay(peribase, 50);

    wr32le(dma_base + 0x00, DMA_CS_RESET);
    udelay(peribase, 100);

    /* Clear interrupt/end flags and set CB address to 0 */
    wr32le(dma_base + 0x04, 0);
    wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END);
}
