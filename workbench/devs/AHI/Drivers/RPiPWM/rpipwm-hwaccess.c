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
#include <exec/memory.h>
#include <aros/macros.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/mbox.h>

#include <hardware/videocore.h>

#include "rpipwm-hwaccess.h"

APTR MBoxBase = NULL;

/*
 * TRUE while the firmware owns the PWM clock. AHI serialises
 * AHIsub_Start/Stop, so a plain flag is enough.
 */
static BOOL pwm_fw_owned = FALSE;

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
 * Fallback: configure the PWM clock directly from PLLD for the desired
 * sample rate, assuming PLLD == PLLD_FREQ (500 MHz).
 *
 * The PWM runs at: clock_freq / range samples per second.
 * So we need: clock_freq = samplerate * range
 * Divisor (12.12 fixed point) = PLLD_FREQ / (samplerate * range)
 *
 * We use MASH=1 (1-stage MASH noise-shaping) for fractional division,
 * which gives better audio quality than integer-only division.
 *
 * NOTE: drifts if the firmware retunes PLLD (display-mode changes);
 * pwm_clock_setup() therefore prefers the firmware path.
 */
static void pwm_clock_setup_cm(ULONG peribase, ULONG samplerate, ULONG range)
{
    ULONG cm_ctl_addr = CM_PWMCTL_ADDR(peribase);
    ULONG cm_div_addr = CM_PWMDIV_ADDR(peribase);
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
 * Single-tag VideoCore property transaction. Request values in vals[]
 * are replaced with the firmware's response on success.
 *
 * Must use MBoxCall (atomic request/response) — a split MBoxWrite +
 * MBoxRead can lose the reply to concurrent mailbox users. The message
 * is confined to one 64-byte cache line so the reply invalidate cannot
 * clash with dirty neighbouring heap data.
 */
#define PWM_FW_MSG_BYTES 64 /* one full cache line, >= 6+3 message words */

static BOOL pwm_fw_property(struct DriverBase *AHIsubBase, ULONG peribase, ULONG tag, ULONG *vals, ULONG nvals)
{
    APTR mbox = (APTR) (peribase + VCMB_OFFSET);
    ULONG msgwords = 6 + nvals; /* header 2, tag header 3, values, end tag */
    ULONG allocsz = PWM_FW_MSG_BYTES + 63;
    ULONG *raw, *m;
    ULONG i;
    BOOL ok = FALSE;

    if (MBoxBase == NULL)
        MBoxBase = OpenResource("mbox.resource");
    if (MBoxBase == NULL || msgwords * 4 > PWM_FW_MSG_BYTES)
        return FALSE;

    raw = AllocMem(allocsz, MEMF_PUBLIC | MEMF_CLEAR);
    if (raw == NULL)
        return FALSE;

    m = (ULONG *) (((IPTR) raw + 63) & ~63);

    m[0] = AROS_LONG2LE(msgwords * 4);
    m[1] = AROS_LONG2LE(VCTAG_REQ);
    m[2] = AROS_LONG2LE(tag);
    m[3] = AROS_LONG2LE(nvals * 4);
    m[4] = AROS_LONG2LE(nvals * 4);
    for (i = 0; i < nvals; i++)
        m[5 + i] = AROS_LONG2LE(vals[i]);
    m[5 + nvals] = 0; /* end tag */

    /*
     * Require our buffer back, a success response code and the
     * tag-processed bit — an error reply leaves the request values
     * in place, spoofing a result.
     */
    if ((APTR) MBoxCall(mbox, VCMB_PROPCHAN, m) == (APTR) m
        && AROS_LE2LONG(m[1]) == VCTAG_RESP
        && (AROS_LE2LONG(m[4]) & VCTAG_RESP)) {
        for (i = 0; i < nvals; i++)
            vals[i] = AROS_LE2LONG(m[5 + i]);
        ok = TRUE;
    }

    FreeMem(raw, allocsz);
    return ok;
}

/* Tell the firmware to turn the PWM clock off. */
static void pwm_fw_release_clock(struct DriverBase *AHIsubBase, ULONG peribase)
{
    ULONG vals[2];

    vals[0] = VCCLOCK_PWM;
    vals[1] = 0; /* bit0 clear = off */
    pwm_fw_property(AHIsubBase, peribase, VCTAG_SETCLKSTATE, vals, 2);
}

/*
 * Have the firmware enable and program the PWM clock. Returns the rate
 * it programmed, or 0 on failure. A firmware-owned clock is re-derived
 * when the parent PLL is retuned, so the rate survives display-mode
 * changes.
 */
static ULONG pwm_fw_set_clock(struct DriverBase *AHIsubBase, ULONG peribase, ULONG rate)
{
    ULONG vals[3];

    /* Enable the clock (only bit0 may be set in a request) */
    vals[0] = VCCLOCK_PWM;
    vals[1] = 1;
    if (!pwm_fw_property(AHIsubBase, peribase, VCTAG_SETCLKSTATE, vals, 2))
        return 0;

    /* Program the clock rate */
    vals[0] = VCCLOCK_PWM;
    vals[1] = rate;
    vals[2] = 0; /* do not skip turbo handling */
    if (!pwm_fw_property(AHIsubBase, peribase, VCTAG_SETCLKRATE, vals, 3)) {
        pwm_fw_release_clock(AHIsubBase, peribase); /* undo the enable above */
        return 0;
    }

    return vals[1]; /* the rate the firmware actually programmed */
}

/*
 * PWM clock rate read back from the CM registers — the mailbox response
 * cannot be trusted (SETCLKRATE on a running clock echoes the new rate
 * while CM_PWMDIV keeps the old divisor). Returns 0 if unverifiable.
 */
static ULONG pwm_cm_actual_clock(ULONG peribase)
{
    ULONG ctl = rd32le(CM_PWMCTL_ADDR(peribase));
    ULONG div = rd32le(CM_PWMDIV_ADDR(peribase)) & 0xFFFFFF; /* 12.12 */

    if ((ctl & 0xF) != CM_SRC_PLLD || div == 0)
        return 0;

    if (((ctl >> 9) & 3) == 0) {
        /* MASH 0: integer division, the fractional field is ignored */
        ULONG divi = div >> 12;
        return divi ? PLLD_FREQ / divi : 0;
    }

    return (ULONG) (((unsigned long long) PLLD_FREQ << 12) / div);
}

/*
 * Accept the firmware rate within 1% — it divides with MASH off, which
 * costs up to ~1% (measured +0.66% at 44100 Hz).
 */
static BOOL pwm_rate_ok(ULONG real, ULONG target)
{
    ULONG diff = (real > target) ? (real - target) : (target - real);
    return real != 0 && diff <= target / 100;
}

/*
 * Configure the PWM clock: prefer the firmware path (stable across
 * display-mode changes), fall back to direct CM programming from PLLD.
 */
void pwm_clock_setup(struct DriverBase *AHIsubBase, ULONG peribase, ULONG samplerate, ULONG range)
{
    ULONG target_freq = samplerate * range;
    ULONG cm_ctl_addr = CM_PWMCTL_ADDR(peribase);
    ULONG got;
    ULONG real = 0;

    /*
     * Stop the clock first: the CM does not latch a divisor written
     * while running, and the firmware's SETCLKRATE does not stop a
     * clock it believes is enabled. SETCLKSTATE(off) also syncs the
     * firmware's cached state.
     */
    wr32le(cm_ctl_addr, CM_PASSWORD | (rd32le(cm_ctl_addr) & ~CM_ENAB));
    while (rd32le(cm_ctl_addr) & CM_BUSY)
        udelay(peribase, 1);
    pwm_fw_release_clock(AHIsubBase, peribase);

    got = pwm_fw_set_clock(AHIsubBase, peribase, target_freq);

    if (got != 0) {
        real = pwm_cm_actual_clock(peribase);

        /*
         * Verify against the registers (see pwm_cm_actual_clock); on
         * mismatch retry once with a full off -> on cycle, giving the
         * firmware a real state transition to act on.
         */
        if (!(rd32le(CM_PWMCTL_ADDR(peribase)) & CM_BUSY) || !pwm_rate_ok(real, target_freq)) {
            bug("[RPiPWM] firmware left PWM clock at %u Hz (want %u); retrying with off/on cycle\n",
                real, target_freq);
            pwm_fw_release_clock(AHIsubBase, peribase);
            got = pwm_fw_set_clock(AHIsubBase, peribase, target_freq);
            real = (got != 0) ? pwm_cm_actual_clock(peribase) : 0;
        }
    }

    if (got != 0 && (rd32le(CM_PWMCTL_ADDR(peribase)) & CM_BUSY)
        && pwm_rate_ok(real, target_freq)) {
        pwm_fw_owned = TRUE;
        return;
    }

    if (got != 0)
        bug("[RPiPWM] firmware path unusable (claimed %u Hz, registers say %u, want %u); using direct PLLD path\n",
            got, real, target_freq);

    /*
     * Release any firmware claim before programming the CM directly, or
     * the firmware would reprogram our divisor on the next PLL retune.
     */
    if (got != 0 || pwm_fw_owned)
        pwm_fw_release_clock(AHIsubBase, peribase);
    pwm_fw_owned = FALSE;

    pwm_clock_setup_cm(peribase, samplerate, range);
}

/*
 * Stop the PWM clock.
 */
void pwm_clock_stop(struct DriverBase *AHIsubBase, ULONG peribase)
{
    ULONG cm_ctl_addr = CM_PWMCTL_ADDR(peribase);

    /*
     * Leave a firmware-owned clock running: releasing it makes the next
     * session's identical enable+rate request be answered from the
     * firmware's cache without the clock actually starting (observed on
     * Pi 3B+). The PWM peripheral itself is stopped, so this is harmless.
     */
    if (pwm_fw_owned)
        return;

    /* Direct disable for the direct-programming path. */
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
        struct BCM2708DMACB *cb = dd->cb[i];

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

    /* The channel is already enabled by dma.resource at allocation. */

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
