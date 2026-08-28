/*
 *  BCM2835 PWM Audio hardware access for Raspberry Pi
 *
 *  Configures the board's audio GPIO pins for PWM output, sets up the clock manager
 *  for the desired sample rate, initializes the PWM peripheral in FIFO
 *  mode with DMA, and manages DMA control blocks for double-buffered
 *  audio playback.
 */

#include <config.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <aros/macros.h>
#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/mbox.h>
#include <proto/openfirmware.h>

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
static void udelay(IPTR peribase, ULONG us)
{
    volatile ULONG *clo = (volatile ULONG *) (IPTR) (peribase + 0x003004);
    ULONG start = AROS_LE2LONG(*clo);

    while ((AROS_LE2LONG(*clo) - start) < us)
        ;
}

/******************************************************************************
** GPIO setup for PWM audio ***************************************************
******************************************************************************/

/*
 * Which pins carry PWM audio is board wiring: a Pi 3 routes the right channel
 * to GPIO 45, a Pi 4 to GPIO 41. The device tree names them, and the function
 * to select, in the GPIO controller's audio_pins node. The values below only
 * stand for a machine that hands us no tree.
 */
#define PWM_GPIO_MAXPINS        4

static ULONG pwm_gpio_pin[PWM_GPIO_MAXPINS] = { 40, 45 };
static ULONG pwm_gpio_count = 2;
static ULONG pwm_gpio_fsel = 4;                 /* ALT0 = PWM0_OUT/PWM1_OUT */

/*
 * Cleared when the tree says this board has no 3.5mm jack: the Pi 400, the
 * Compute Modules and the Zero 2 W keep the audio_pins node but leave its pin
 * list empty, and the Pi 5/500 have no such node at all. A machine that hands
 * us no tree keeps the defaults above and stays playable.
 */
static BOOL pwm_audio_wired = TRUE;

static void pwm_gpio_query(struct DriverBase *AHIsubBase)
{
    static BOOL asked = FALSE;
    void *OpenFirmwareBase;
    void *node, *prop;
    ULONG *cells;
    ULONG i, n;

    if (asked)
        return;
    asked = TRUE;

    if ((OpenFirmwareBase = OpenResource("openfirmware.resource")) == NULL)
        return;

    /* audio_pins has no compatible string, so only the symbol table
     * records where it sits. */
    node = OF_OpenKey("/__symbols__");
    prop = node ? OF_FindProperty(node, "audio_pins") : NULL;
    node = prop ? OF_OpenKey(OF_GetPropValue(prop)) : NULL;
    if (node == NULL)
    {
        pwm_audio_wired = FALSE;
        return;
    }

    prop = OF_FindProperty(node, "brcm,pins");
    cells = prop ? OF_GetPropValue(prop) : NULL;
    n = prop ? OF_GetPropLen(prop) / sizeof(ULONG) : 0;
    if (n == 0 || cells == NULL)
    {
        pwm_audio_wired = FALSE;
        return;
    }

    if (n > PWM_GPIO_MAXPINS)
        return;

    for (i = 0; i < n; i++)
        pwm_gpio_pin[i] = AROS_BE2LONG(cells[i]);
    pwm_gpio_count = n;

    /* One function value covers the whole list. */
    prop = OF_FindProperty(node, "brcm,function");
    cells = prop ? OF_GetPropValue(prop) : NULL;
    if (cells != NULL && OF_GetPropLen(prop) >= sizeof(ULONG))
        pwm_gpio_fsel = AROS_BE2LONG(cells[0]) & 7;
}

BOOL pwm_audio_present(struct DriverBase *AHIsubBase)
{
    pwm_gpio_query(AHIsubBase);

    return pwm_audio_wired;
}

/*
 * Point the audio pins at the PWM block; they reach the 3.5mm jack through an
 * RC filter on the board. GPFSELn holds ten three-bit fields. Usually a no-op,
 * the firmware having selected this already.
 */
void pwm_gpio_setup(struct DriverBase *AHIsubBase, IPTR peribase)
{
    ULONG i;

    pwm_gpio_query(AHIsubBase);

    for (i = 0; i < pwm_gpio_count; i++)
    {
        ULONG pin = pwm_gpio_pin[i];
        IPTR gpfsel = peribase + 0x200000 + (pin / 10) * 4;
        ULONG shift = (pin % 10) * 3;
        ULONG val = rd32le(gpfsel);

        val &= ~(7 << shift);
        val |= pwm_gpio_fsel << shift;
        wr32le(gpfsel, val);
    }
}

/*
 * The pins are never put back to input. A disabled PWM block drives them low,
 * matching the firmware's idle level; an input pin floats instead and the
 * jack's coupling capacitor drifts. Muxing a floating node back onto a driven
 * one cannot be slewed, so that step is a click no ramp can hide.
 */

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
/* The PLL rate the clock manager divides down on this SoC. */
static ULONG plld_freq(IPTR peribase)
{
    return (peribase == BCM2711_PERIIOBASE) ? PLLD_FREQ_BCM2711 : PLLD_FREQ;
}

static void pwm_clock_setup_cm(IPTR peribase, ULONG samplerate, ULONG range)
{
    ULONG cm_ctl_addr = CM_PWMCTL_ADDR(peribase);
    ULONG cm_div_addr = CM_PWMDIV_ADDR(peribase);
    ULONG plld = plld_freq(peribase);
    ULONG target_freq;
    ULONG divi, divf;

    /* Stop the clock */
    wr32le(cm_ctl_addr, CM_PASSWORD | (rd32le(cm_ctl_addr) & ~CM_ENAB));

    /* Wait for clock to stop (BUSY flag clears) */
    while (rd32le(cm_ctl_addr) & CM_BUSY)
        udelay(peribase, 1);

    /* Calculate divisor: 12-bit integer, 12-bit fraction */
    target_freq = samplerate * range;
    divi = plld / target_freq;
    divf = ((ULONG) ((((unsigned long long) (plld % target_freq)) << 12) / target_freq)) & 0xFFF;

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

static BOOL pwm_fw_property(struct DriverBase *AHIsubBase, IPTR peribase, ULONG tag, ULONG *vals, ULONG nvals)
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
static void pwm_fw_release_clock(struct DriverBase *AHIsubBase, IPTR peribase)
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
static ULONG pwm_fw_set_clock(struct DriverBase *AHIsubBase, IPTR peribase, ULONG rate)
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
static ULONG pwm_cm_actual_clock(IPTR peribase)
{
    ULONG ctl = rd32le(CM_PWMCTL_ADDR(peribase));
    ULONG div = rd32le(CM_PWMDIV_ADDR(peribase)) & 0xFFFFFF; /* 12.12 */
    ULONG plld = plld_freq(peribase);

    if ((ctl & 0xF) != CM_SRC_PLLD || div == 0)
        return 0;

    if (((ctl >> 9) & 3) == 0) {
        /* MASH 0: integer division, the fractional field is ignored */
        ULONG divi = div >> 12;
        return divi ? plld / divi : 0;
    }

    return (ULONG) (((unsigned long long) plld << 12) / div);
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
 * Start the generator if the firmware left it stopped. The CM will not latch a
 * divisor while running, so the firmware stops it to write one and does not
 * always start it again - a correct rate with BUSY clear reads as a failed
 * firmware path. Only ENAB is touched, so the firmware still owns the rate.
 */
static void pwm_cm_start(IPTR peribase)
{
    ULONG cm_ctl_addr = CM_PWMCTL_ADDR(peribase);
    ULONG timeout = 100;

    if (rd32le(cm_ctl_addr) & CM_BUSY)
        return;

    wr32le(cm_ctl_addr, CM_PASSWORD | (rd32le(cm_ctl_addr) | CM_ENAB));
    while (!(rd32le(cm_ctl_addr) & CM_BUSY) && timeout--)
        udelay(peribase, 10);
}

/*
 * Configure the PWM clock: prefer the firmware path (stable across
 * display-mode changes), fall back to direct CM programming from PLLD.
 */
void pwm_clock_setup(struct DriverBase *AHIsubBase, IPTR peribase, ULONG samplerate, ULONG range)
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
        pwm_cm_start(peribase);
        real = pwm_cm_actual_clock(peribase);

        /*
         * Verify against the registers (see pwm_cm_actual_clock); on
         * mismatch retry once with a full off -> on cycle, giving the
         * firmware a real state transition to act on.
         */
        if (!(rd32le(cm_ctl_addr) & CM_BUSY) || !pwm_rate_ok(real, target_freq)) {
            D(bug("[RPiPWM] firmware left PWM clock at %u Hz (want %u); retrying with off/on cycle\n",
                real, target_freq));
            pwm_fw_release_clock(AHIsubBase, peribase);
            got = pwm_fw_set_clock(AHIsubBase, peribase, target_freq);
            if (got != 0) {
                pwm_cm_start(peribase);
                real = pwm_cm_actual_clock(peribase);
            } else
                real = 0;
        }
    }

    if (got != 0 && (rd32le(CM_PWMCTL_ADDR(peribase)) & CM_BUSY)
        && pwm_rate_ok(real, target_freq)) {
        pwm_fw_owned = TRUE;
        return;
    }

    /*
     * Not a warning on a BCM2711: with MASH off the nearest integer divisor of
     * a 750 MHz PLLD is 3% away at these rates, so the direct path is simply
     * the right answer there - and logging stalls the serial port for
     * milliseconds right before the audio ramp.
     */
    if (got != 0)
        D(bug("[RPiPWM] firmware path unusable (claimed %u Hz, registers say %u, want %u); using direct PLLD path\n",
            got, real, target_freq));

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
void pwm_clock_stop(struct DriverBase *AHIsubBase, IPTR peribase)
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
 * Initialize both PWM channels for audio output, Mark/Space mode. PWEN goes on
 * here because the PWM DREQ only asserts while the channels are active.
 *
 * They start on the data register at 0% duty, not the FIFO: the output is a DC
 * level and every abrupt change in it is a click. 0% matches an undriven pin,
 * so the mux can be switched without a step; pwm_ramp_dc() then walks the
 * level up below hearing and pwm_fifo_enable() hands over to the DMA.
 */
void pwm_init(IPTR peribase, ULONG range)
{
    IPTR pwm_base = pwm_block(peribase);

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

    /* Silent starting level, transmitted from the data register. */
    wr32le(pwm_base + 0x14, 0);         /* PWM_DAT1 */
    wr32le(pwm_base + 0x24, 0);         /* PWM_DAT2 */

    /* Enable both channels:
     * PWEN1/2 = enable channel
     * No MODE bit = PWM mode (not serializer) for analog output
     * MSEN1/2 = Mark/Space mode for better audio quality
     * USEF1/2 stays clear until pwm_fifo_enable()
     */
    wr32le(pwm_base + 0x00,
           PWM_CTL_PWEN1 | PWM_CTL_MSEN1 | PWM_CTL_PWEN2 | PWM_CTL_MSEN2);
}

/*
 * Prepare an already-biased block for a new session without touching PWEN or
 * the DC. Range and data scale together, so the output level does not move.
 */
void pwm_rearm(IPTR peribase, ULONG range)
{
    IPTR pwm_base = pwm_block(peribase);
    ULONG ctl = rd32le(pwm_base + 0x00);

    wr32le(pwm_base + 0x10, range);             /* PWM_RNG1 */
    wr32le(pwm_base + 0x20, range);             /* PWM_RNG2 */
    wr32le(pwm_base + 0x14, range / 2);         /* PWM_DAT1 */
    wr32le(pwm_base + 0x24, range / 2);         /* PWM_DAT2 */

    /* Empty the FIFO and clear stale status, keeping the enables. */
    wr32le(pwm_base + 0x00, ctl | PWM_CTL_CLRF1);
    wr32le(pwm_base + 0x04, 0xFFFFFFFF);

    wr32le(pwm_base + 0x08, PWM_DMAC_ENAB | PWM_DMAC_PANIC(7) | PWM_DMAC_DREQ(3));
}

/*
 * Walk the DC level between two duty values. 64 steps of a millisecond keeps
 * the slew below the audible band, so the coupling capacitor charges quietly.
 */
#define PWM_RAMP_STEPS  64
#define PWM_RAMP_US     1000

void pwm_ramp_dc(IPTR peribase, ULONG from, ULONG to)
{
    IPTR pwm_base = pwm_block(peribase);
    LONG span = (LONG) to - (LONG) from;
    ULONG i;

    for (i = 1; i <= PWM_RAMP_STEPS; i++) {
        ULONG v = (ULONG) ((LONG) from + span * (LONG) i / PWM_RAMP_STEPS);

        wr32le(pwm_base + 0x14, v);     /* PWM_DAT1 */
        wr32le(pwm_base + 0x24, v);     /* PWM_DAT2 */
        udelay(peribase, PWM_RAMP_US);
    }
}

/*
 * Hand the channels over to the FIFO. The DMA has already filled it with the
 * level the data register is holding, so the switch is not a step.
 */
void pwm_fifo_enable(IPTR peribase)
{
    IPTR pwm_base = pwm_block(peribase);

    wr32le(pwm_base + 0x00,
           rd32le(pwm_base + 0x00) | PWM_CTL_USEF1 | PWM_CTL_USEF2);
}

/*
 * Take the channels back off the FIFO and hold a fixed level, so the DC can be
 * ramped down before the pins are released.
 */
void pwm_dat_hold(IPTR peribase, ULONG value)
{
    IPTR pwm_base = pwm_block(peribase);

    wr32le(pwm_base + 0x14, value);     /* PWM_DAT1 */
    wr32le(pwm_base + 0x24, value);     /* PWM_DAT2 */
    wr32le(pwm_base + 0x00,
           rd32le(pwm_base + 0x00) & ~(PWM_CTL_USEF1 | PWM_CTL_USEF2));
}

/*
 * The level on the pin right now, read from where the engine has got to. The
 * mixer runs a buffer ahead, so its last converted sample can be 20 ms off -
 * ramping down from there leaves the step the ramp exists to avoid. Call while
 * SOURCE_AD is still live, before stopping the channel.
 */
ULONG dma_current_level(struct RPiPWMData *dd)
{
    IPTR dma_base = dd->periiobase + 0x007000 + dd->dma_channel * 0x100;
    IPTR src = rd32le(dma_base + 0x0C) & ~0xC0000000U;  /* SOURCE_AD, bus addr */
    int i;

    for (i = 0; i < 2; i++) {
        IPTR base = (IPTR) dd->dmabuf[i];

        /* One word back: SOURCE_AD has already been advanced past it. */
        if (src > base && src <= base + dd->dmabuf_size)
            return AROS_LE2LONG(*(volatile ULONG *) (src - sizeof(ULONG)));
    }

    return dd->pwm_range / 2;
}

/*
 * Stop the PWM peripheral.
 */
void pwm_stop(IPTR peribase)
{
    IPTR pwm_base = pwm_block(peribase);

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
#if 0
/*
 * How DMA_DREQ_PWM_2711 was found, kept for the next SoC that moves it. No
 * device tree names a DREQ for the block behind GPIO 40/41, so measure: aim a
 * one-shot transfer at the FIFO with each candidate and time it. Another busy
 * peripheral's DREQ also shifts data, but only the right one does two words
 * per frame at the mixing rate.
 *
 * Use in place of the constant with expect = ahiac_MixFreq * 2 / 100, and
 * rebuild the control blocks after - it clobbers cb[0]. On a Pi 4: permap 1
 * moved 891 words in 10 ms against ~882 wanted, and nothing else moved.
 */
ULONG dma_probe_dreq(struct DriverBase *AHIsubBase, struct RPiPWMData *dd, ULONG expect)
{
    IPTR peribase = dd->periiobase;
    IPTR dma_base = peribase + 0x007000 + dd->dma_channel * 0x100;
    ULONG len = dd->dmabuf_size;
    ULONG best = DMA_DREQ_PWM;
    ULONG best_err = ~0U;
    ULONG n;

    for (n = 1; n < 32; n++) {
        ULONG left, moved, err;

        wr32le(dma_base + 0x00, DMA_CS_RESET);
        udelay(peribase, 100);

        dd->cb[0]->ti = DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ | DMA_TI_SRC_INC |
                        DMA_TI_PERMAP(n) | DMA_TI_NO_WIDE_BURSTS;
        dd->cb[0]->source_ad = GPU_BUS_ADDR(dd->dmabuf[0]);
        dd->cb[0]->dest_ad = pwm_fifo_bus(peribase);
        dd->cb[0]->txfr_len = len;
        dd->cb[0]->stride = 0;
        dd->cb[0]->nextconbk = 0;
        CacheClearE(dd->cb[0], sizeof(struct BCM2708DMACB), CACRF_ClearD);

        wr32le(dma_base + 0x04, GPU_BUS_ADDR(dd->cb[0]));
        wr32le(dma_base + 0x00, DMA_CS_ACTIVE);

        udelay(peribase, 10000);

        left = rd32le(dma_base + 0x14);          /* TXFR_LEN, counts down */
        wr32le(dma_base + 0x00, DMA_CS_RESET);

        moved = (left < len) ? (len - left) / sizeof(ULONG) : 0;
        if (moved == 0)
            continue;

        err = (moved > expect) ? (moved - expect) : (expect - moved);
        bug("[RPiPWM] dreq probe: permap %u moved %u words in 10 ms (want ~%u)\n",
            n, moved, expect);

        if (err < best_err) {
            best_err = err;
            best = n;
        }
    }

    /* Within a quarter of the expected rate is the paced one; anything else is
     * some other peripheral's request line and no use to us. */
    if (best_err > expect / 4) {
        bug("[RPiPWM] dreq probe: nothing paced at ~%u words/10 ms, keeping %u\n",
            expect, DMA_DREQ_PWM);
        best = DMA_DREQ_PWM;
    } else
        bug("[RPiPWM] dreq probe: using permap %u\n", best);

    return best;
}
#endif

void dma_build_control_blocks(struct RPiPWMData *dd, IPTR peribase)
{
    int i;

    for (i = 0; i < 2; i++) {
        struct BCM2708DMACB *cb = dd->cb[i];

        cb->ti = DMA_TI_INTEN | DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ | DMA_TI_SRC_INC | DMA_TI_PERMAP(pwm_dreq(peribase)) |
                 DMA_TI_NO_WIDE_BURSTS;

        cb->source_ad = GPU_BUS_ADDR(dd->dmabuf[i]);
        cb->dest_ad = pwm_fifo_bus(peribase);
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
void dma_setup(IPTR peribase, ULONG channel, ULONG cb_bus_addr)
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

    /* Start DMA. The IRQ handler re-writes this same value with the W1C flags
     * added, so the priorities survive the session; see bcm2708_dma.h. */
    wr32le(dma_base + 0x00, BCM2708_DMA_CS_RUN);
}

/*
 * Stop DMA on the specified channel.
 */
void dma_stop(IPTR peribase, ULONG channel)
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
