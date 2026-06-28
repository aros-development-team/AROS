/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    sdio.resource - SDIO host bound to the BCM2835 Arasan SDHCI controller.

    On Raspberry Pi 3/3B/4 the Arasan controller (peripheral offset 0x300000)
    is wired to the on-board Broadcom WiFi chip over a 4-bit SDIO bus, while
    the SD card uses the separate SDHOST controller. This resource brings up
    the Arasan controller, performs SDIO card initialisation (CMD5/CMD3/CMD7)
    and exposes a generic CMD52/CMD53 function-I/O API for client drivers.

    The low-level command issue mirrors the proven sequence in
    rom/devs/sdcard/sdcard_bus.c (the BCM2835 Arasan needs an atomic 32-bit
    TRANSFER_MODE+COMMAND write and a short delay between register writes),
    but runs fully polled - no IRQ is routed to the ARM (SIGNAL_ENABLE = 0).
*/

#define DEBUG 0

#include <aros/macros.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/mbox.h>
#include <proto/sdio.h>

#include <hardware/sdhc.h>
#include <hardware/mmc.h>
#include <hardware/videocore.h>

#include "sdio_private.h"

APTR KernelBase __attribute__((used)) = NULL;
APTR MBoxBase __attribute__((used)) = NULL;

/* VideoCore property-channel constants (not in the public videocore.h;
 * mirrored from the sdcard driver's intern header). */
#define VCMB_PROPCHAN           8
#define VCPOWER_SDHCI           0
#define VCPOWER_STATE_ON        (1 << 0)
#define VCPOWER_STATE_WAIT      (1 << 1)
#define VCCLOCK_SDHCI           1

/*
 * VideoCore firmware GPIO-expander property tags. On Pi 3/3B+/4 the WiFi
 * enable line (WL_REG_ON / "WL_ON") is not a SoC GPIO but expander pin 1,
 * reachable only through the firmware. Expander pins are addressed at an
 * offset of RPI_EXP_GPIO_BASE in the firmware GPIO numbering; driving
 * WL_ON high powers up the WiFi chip. These tag values and the config
 * payload order are part of the VideoCore firmware mailbox ABI.
 */
#define RPI_FW_SET_GPIO_CONFIG  0x00038043
#define RPI_FW_SET_GPIO_STATE   0x00038041
#define RPI_FW_GET_GPIO_STATE   0x00030041
#define RPI_FW_TAG_RESP         0x80000000      /* set in tag size word when handled */
#define RPI_EXP_GPIO_BASE       128
#define RPI_EXP_GPIO_WL_ON      1
#define RPI_EXP_GPIO_DIR_OUT    1

/*
 * The BCM434xx combo chip needs a 32.768 kHz low-power oscillator (LPO)
 * clock to run its internal logic, including the SDIO core. On the Pi
 * 3/3B+ the SoC supplies it on GPCLK2 (GPIO43), but the firmware does not
 * start it for us: GPIO43 comes up as a plain input. Without the LPO the
 * chip stays asleep and never answers CMD5.
 * Derive ~32.768 kHz from the 19.2 MHz crystal with the MASH-1 fractional
 * divider: 19.2e6 / 585.9375 = 32768 Hz (DIVI 585, DIVF 0.9375*4096=3840).
 */
#define CM_GP2CTL               (CLOCK_BASE + 0x80)
#define CM_GP2DIV               (CLOCK_BASE + 0x84)
#define SDIO_LPO_DIVI           585
#define SDIO_LPO_DIVF           3840

/* MMC command opcodes reused from the memory-card set */
#define SDIO_CMD_SEND_RELATIVE_ADDR     MMC_CMD_SET_RELATIVE_ADDR       /* CMD3, R6 */
#define SDIO_CMD_SELECT_CARD            MMC_CMD_SELECT_CARD             /* CMD7, R1b */

#define SDIO_IDENT_CLOCK                400000          /* 400 kHz identification clock */
#define SDIO_FULL_CLOCK                 25000000        /* post-init bus clock target */

/* ----------------------------------------------------------------------- */
/* Timing                                                                  */

static inline ULONG sdio_now(struct SDIOBase *SDIOBase)
{
    return AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO);
}

static void sdio_udelay(struct SDIOBase *SDIOBase, ULONG us)
{
    ULONG start = sdio_now(SDIOBase);

    while ((sdio_now(SDIOBase) - start) < us)
        ;
}

/* ----------------------------------------------------------------------- */
/* MMIO accessors (SDHCI registers are 8/16/32-bit; the bus requires       */
/* 32-bit aligned access, and back-to-back writes need a short gap).       */

static UBYTE sdio_rb(struct SDIOBase *SDIOBase, ULONG reg)
{
    ULONG val = AROS_LE2LONG(*(volatile ULONG *)((SDIOBase->sdio_iobase + reg) & ~3));
    return (val >> ((reg & 3) << 3)) & 0xFF;
}

static UWORD sdio_rw(struct SDIOBase *SDIOBase, ULONG reg)
{
    ULONG val = AROS_LE2LONG(*(volatile ULONG *)((SDIOBase->sdio_iobase + reg) & ~3));
    return (val >> (((reg >> 1) & 1) << 4)) & 0xFFFF;
}

static ULONG sdio_rl(struct SDIOBase *SDIOBase, ULONG reg)
{
    return AROS_LE2LONG(*(volatile ULONG *)(SDIOBase->sdio_iobase + reg));
}

static void sdio_wl(struct SDIOBase *SDIOBase, ULONG reg, ULONG val)
{
    /* BCM2835 Arasan erratum: two SD-clock cycles must elapse between
     * successive controller writes. At the 400 kHz identification clock
     * that is 5 us; use 6 us to match the proven sdcard_bcm2708bus.c. */
    while ((sdio_now(SDIOBase) - SDIOBase->sdio_LastWrite) < 6)
        ;

    *(volatile ULONG *)(SDIOBase->sdio_iobase + reg) = AROS_LONG2LE(val);
    SDIOBase->sdio_LastWrite = sdio_now(SDIOBase);
}

static void sdio_wb(struct SDIOBase *SDIOBase, ULONG reg, UBYTE val)
{
    ULONG cur = AROS_LE2LONG(*(volatile ULONG *)((SDIOBase->sdio_iobase + reg) & ~3));
    ULONG shift = (reg & 3) << 3;
    ULONG mask = 0xFF << shift;

    sdio_wl(SDIOBase, reg & ~3, (cur & ~mask) | (val << shift));
}

static void sdio_ww(struct SDIOBase *SDIOBase, ULONG reg, UWORD val)
{
    ULONG cur = AROS_LE2LONG(*(volatile ULONG *)((SDIOBase->sdio_iobase + reg) & ~3));
    ULONG shift = ((reg >> 1) & 1) << 4;
    ULONG mask = 0xFFFF << shift;

    sdio_wl(SDIOBase, reg & ~3, (cur & ~mask) | (val << shift));
}

/*
 * Raw 32-bit access to the PIO data port (SDHCI_BUFFER). The inter-write
 * erratum delay in sdio_wl applies to *control* registers only; the data
 * FIFO must be filled/drained at full speed (the reference driver polls
 * with no per-word delay), so these bypass it.
 */
static inline ULONG sdio_fifo_r(struct SDIOBase *SDIOBase)
{
    return AROS_LE2LONG(*(volatile ULONG *)(SDIOBase->sdio_iobase + SDHCI_BUFFER));
}

static inline void sdio_fifo_w(struct SDIOBase *SDIOBase, ULONG val)
{
    *(volatile ULONG *)(SDIOBase->sdio_iobase + SDHCI_BUFFER) = AROS_LONG2LE(val);
}

/* ----------------------------------------------------------------------- */
/* Controller bring-up                                                     */

static void sdio_softreset(struct SDIOBase *SDIOBase, UBYTE mask)
{
    ULONG timeout = 100;

    sdio_wb(SDIOBase, SDHCI_RESET, mask);
    while (sdio_rb(SDIOBase, SDHCI_RESET) & mask)
    {
        if (timeout-- == 0)
        {
            D(bug("[SDIO] reset timeout (mask 0x%02x)\n", mask));
            break;
        }
        sdio_udelay(SDIOBase, 1000);
    }
}

static void sdio_setclock(struct SDIOBase *SDIOBase, ULONG speed)
{
    ULONG n, timeout;
    UWORD ctrl;

    /*
     * SDHCI clock = base / (2 * N). The BCM2835 Arasan core requires the
     * divisor N to be a power of two (a linear value leaves multiple bits
     * set in the frequency-select field and produces a bad/no SD clock).
     * Pick the smallest power-of-two N giving <= speed.
     */
    n = 1;
    while ((SDIOBase->sdio_ClockMax / (2 * n)) > speed && n < 0x400)
        n <<= 1;          /* 10-bit divider (8 low + 2 high), so up to 0x3ff */

    ctrl = ((n & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT) |
           (((n >> SDHCI_DIV_MASK_LEN) & 0x3) << SDHCI_DIVIDER_HI_SHIFT);

    D(bug("[SDIO] clock divisor N=%u -> ~%u Hz (ctrl 0x%04x)\n",
          n, SDIOBase->sdio_ClockMax / (2 * n), ctrl));

    sdio_ww(SDIOBase, SDHCI_CLOCK_CONTROL, 0);
    sdio_ww(SDIOBase, SDHCI_CLOCK_CONTROL, ctrl | SDHCI_CLOCK_INT_EN);

    timeout = 20;
    while (!(sdio_rw(SDIOBase, SDHCI_CLOCK_CONTROL) & SDHCI_CLOCK_INT_STABLE))
    {
        if (timeout-- == 0)
        {
            D(bug("[SDIO] clock failed to stabilise\n"));
            break;
        }
        sdio_udelay(SDIOBase, 1000);
    }

    ctrl = sdio_rw(SDIOBase, SDHCI_CLOCK_CONTROL) | SDHCI_CLOCK_CARD_EN;
    sdio_ww(SDIOBase, SDHCI_CLOCK_CONTROL, ctrl);
}

static void sdio_setpower(struct SDIOBase *SDIOBase)
{
    sdio_wb(SDIOBase, SDHCI_POWER_CONTROL, SDHCI_POWER_330);
    sdio_wb(SDIOBase, SDHCI_POWER_CONTROL, SDHCI_POWER_330 | SDHCI_POWER_ON);
    sdio_udelay(SDIOBase, 10000);       /* let the rail ramp before the first CMD */
}

/*
 * Route GPIO 34-39 to ALT3 (= SD1 / Arasan SDHCI), the WiFi SDIO bus on
 * Pi 3/3B/4. The SD card uses GPIO 48-53 on the SDHOST controller and is
 * untouched here.
 */
static void sdio_gpio_mux(struct SDIOBase *SDIOBase)
{
    volatile ULONG *fsel3 = (volatile ULONG *)GPFSEL3;
    ULONG val = AROS_LE2LONG(*fsel3);
    int pin;

    /* GPIO 34..39 occupy fields 4..9 of GPFSEL3 (3 bits each). The function
     * select encoding for ALT3 is 0b111 (7). */
    for (pin = 34; pin <= 39; pin++)
    {
        int shift = (pin % 10) * 3;
        val &= ~(0x7 << shift);
        val |= (0x7 << shift);          /* ALT3 = SD1 / Arasan SDHCI */
    }
    *fsel3 = AROS_LONG2LE(val);

    D(bug("[SDIO] GPIO34-39 -> ALT3 (GPFSEL3=0x%08x)\n", val));
}

/*
 * Pull configuration for the SDIO bus, matching the Pi device tree:
 * CLK (GPIO34) no pull, CMD/DAT (GPIO35-39) pull-up. Uses the legacy
 * BCM2835 GPPUD/GPPUDCLK clocked sequence (bank 1 = GPIO32-53).
 */
static void sdio_gpio_pulls(struct SDIOBase *SDIOBase)
{
    volatile ULONG *gppud = (volatile ULONG *)GPPUD;
    volatile ULONG *clk1 = (volatile ULONG *)GPPUDCLK1;

    /* GPIO35-39: pull-up (GPPUD = 2) */
    *gppud = AROS_LONG2LE(2);
    sdio_udelay(SDIOBase, 1);
    *clk1 = AROS_LONG2LE((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7));
    sdio_udelay(SDIOBase, 1);
    *gppud = AROS_LONG2LE(0);
    *clk1 = AROS_LONG2LE(0);

    /* GPIO34: no pull (GPPUD = 0) */
    *gppud = AROS_LONG2LE(0);
    sdio_udelay(SDIOBase, 1);
    *clk1 = AROS_LONG2LE(1 << 2);
    sdio_udelay(SDIOBase, 1);
    *gppud = AROS_LONG2LE(0);
    *clk1 = AROS_LONG2LE(0);
}

/*
 * Start the 32.768 kHz LPO clock the WiFi chip runs from, on GPCLK2/GPIO43.
 * Must run before WL_REG_ON so the clock is stable when the chip powers up.
 */
static void sdio_wifi_clock(struct SDIOBase *SDIOBase)
{
    volatile ULONG *ctl = (volatile ULONG *)CM_GP2CTL;
    volatile ULONG *divr = (volatile ULONG *)CM_GP2DIV;
    volatile ULONG *fsel4 = (volatile ULONG *)GPFSEL4;
    int shift = (43 % 10) * 3;          /* GPIO43 = field 3 of GPFSEL4 */
    ULONG val, timeout;

    D(bug("[SDIO] GPCLK2 before: CTL=0x%08x DIV=0x%08x GPFSEL4=0x%08x\n",
          AROS_LE2LONG(*ctl), AROS_LE2LONG(*divr), AROS_LE2LONG(*fsel4)));

    /* Stop the clock generator and wait for it to go idle */
    *ctl = AROS_LONG2LE(CM_PASSWORD | CM_SRC_OSC);
    timeout = 1000;
    while ((AROS_LE2LONG(*ctl) & CM_BUSY) && timeout--)
        sdio_udelay(SDIOBase, 10);

    /* Fractional (MASH-1) divider from the 19.2 MHz crystal */
    *divr = AROS_LONG2LE(CM_PASSWORD | (SDIO_LPO_DIVI << 12) | SDIO_LPO_DIVF);

    /* Select source + MASH, then enable in a second write (per datasheet) */
    *ctl = AROS_LONG2LE(CM_PASSWORD | CM_MASH(1) | CM_SRC_OSC);
    *ctl = AROS_LONG2LE(CM_PASSWORD | CM_MASH(1) | CM_SRC_OSC | CM_ENAB);
    timeout = 1000;
    while (!(AROS_LE2LONG(*ctl) & CM_BUSY) && timeout--)
        sdio_udelay(SDIOBase, 10);

    /* Route GPIO43 to ALT0 = GPCLK2 */
    val = AROS_LE2LONG(*fsel4);
    val &= ~(0x7 << shift);
    val |= (0x4 << shift);              /* ALT0 */
    *fsel4 = AROS_LONG2LE(val);

    D(bug("[SDIO] GPCLK2 32kHz LPO on GPIO43: CTL=0x%08x DIV=0x%08x GPFSEL4=0x%08x\n",
          AROS_LE2LONG(*ctl), AROS_LE2LONG(*divr), AROS_LE2LONG(*fsel4)));
}

/*
 * Power up the WiFi chip by driving WL_REG_ON (firmware expander pin 1)
 * high via the VideoCore property mailbox. Without this the chip stays
 * unpowered and never answers CMD5.
 */
static void sdio_wifi_power(struct SDIOBase *SDIOBase)
{
    unsigned int *raw = AllocMem(16 * 4 + 16, MEMF_PUBLIC | MEMF_CLEAR);
    unsigned int *msg;
    unsigned int gpio = RPI_EXP_GPIO_BASE + RPI_EXP_GPIO_WL_ON;
    int state;

    if (!raw)
        return;
    msg = (unsigned int *)((((IPTR)raw) + 15) & ~15);

    /* Configure the expander pin as an output (initial level low) */
    msg[0] = AROS_LONG2LE(12 * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(RPI_FW_SET_GPIO_CONFIG);
    msg[3] = AROS_LONG2LE(24);
    msg[4] = 0;                                     /* req/resp code: 0 on request */
    msg[5] = AROS_LONG2LE(gpio);
    msg[6] = AROS_LONG2LE(RPI_EXP_GPIO_DIR_OUT);    /* direction */
    msg[7] = AROS_LONG2LE(0);                       /* polarity */
    msg[8] = AROS_LONG2LE(0);                       /* term_en */
    msg[9] = AROS_LONG2LE(0);                       /* term_pull_up */
    msg[10] = AROS_LONG2LE(0);                      /* state */
    msg[11] = 0;
    MBoxCall((APTR)VCMB_BASE, VCMB_PROPCHAN, msg);
    D(bug("[SDIO] SET_GPIO_CONFIG resp 0x%08x tag 0x%08x\n",
          AROS_LE2LONG(msg[1]), AROS_LE2LONG(msg[4])));

    /* Power cycle WL_REG_ON: drive low, settle, then high */
    for (state = 0; state <= 1; state++)
    {
        msg[0] = AROS_LONG2LE(8 * 4);
        msg[1] = AROS_LONG2LE(VCTAG_REQ);
        msg[2] = AROS_LONG2LE(RPI_FW_SET_GPIO_STATE);
        msg[3] = AROS_LONG2LE(8);
        msg[4] = 0;                                 /* req/resp code: 0 on request */
        msg[5] = AROS_LONG2LE(gpio);
        msg[6] = AROS_LONG2LE(state);
        msg[7] = 0;
        MBoxCall((APTR)VCMB_BASE, VCMB_PROPCHAN, msg);
        D(bug("[SDIO] SET_GPIO_STATE %d resp 0x%08x tag 0x%08x\n",
              state, AROS_LE2LONG(msg[1]), AROS_LE2LONG(msg[4])));
        sdio_udelay(SDIOBase, 10000);
    }

    /* Read the pin back to confirm the firmware actually drives it */
    msg[0] = AROS_LONG2LE(8 * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(RPI_FW_GET_GPIO_STATE);
    msg[3] = AROS_LONG2LE(8);
    msg[4] = 0;                                     /* req/resp code: 0 on request */
    msg[5] = AROS_LONG2LE(gpio);
    msg[6] = 0;
    msg[7] = 0;
    MBoxCall((APTR)VCMB_BASE, VCMB_PROPCHAN, msg);
    D(bug("[SDIO] GET_GPIO_STATE gpio %u -> state %u (tag 0x%08x)\n",
          gpio, AROS_LE2LONG(msg[6]), AROS_LE2LONG(msg[4])));

    FreeMem(raw, 16 * 4 + 16);

    D(bug("[SDIO] WL_REG_ON sequence done\n"));
}

/* ----------------------------------------------------------------------- */
/* VideoCore mailbox: power the Arasan controller and read its base clock  */

static int sdio_mbox_setup(struct SDIOBase *SDIOBase)
{
    unsigned int *raw = AllocMem(8 * 4 + 16, MEMF_PUBLIC | MEMF_CLEAR);
    unsigned int *msg = (unsigned int *)((((IPTR)raw) + 15) & ~15);
    int ok = FALSE;

    if (!raw)
        return FALSE;

    /* Power on the SDHCI controller */
    msg[0] = AROS_LONG2LE(8 * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(VCTAG_SETPOWER);
    msg[3] = AROS_LONG2LE(8);
    msg[4] = AROS_LONG2LE(8);
    msg[5] = AROS_LONG2LE(VCPOWER_SDHCI);
    msg[6] = AROS_LONG2LE(VCPOWER_STATE_ON | VCPOWER_STATE_WAIT);
    msg[7] = 0;
    MBoxCall((APTR)VCMB_BASE, VCMB_PROPCHAN, msg);

    /*
     * Enable the EMMC/SDHCI clock (id 1). AROS drives the SD card via the
     * SDHOST controller, so the firmware may have gated the Arasan clock
     * off - without it the controller has no clock to drive the bus and
     * every command times out. (WiFiPi does set_clock_state(1,1) here.)
     */
    msg[0] = AROS_LONG2LE(8 * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(VCTAG_SETCLKSTATE);
    msg[3] = AROS_LONG2LE(8);
    msg[4] = AROS_LONG2LE(8);
    msg[5] = AROS_LONG2LE(VCCLOCK_SDHCI);
    msg[6] = AROS_LONG2LE(1);                   /* bit0 = enable */
    msg[7] = 0;
    MBoxCall((APTR)VCMB_BASE, VCMB_PROPCHAN, msg);
    D(bug("[SDIO] SETCLKSTATE resp 0x%08x state 0x%08x\n",
          AROS_LE2LONG(msg[1]), AROS_LE2LONG(msg[6])));

    /* Query the controller base clock */
    msg[0] = AROS_LONG2LE(8 * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(VCTAG_GETCLKRATE);
    msg[3] = AROS_LONG2LE(8);
    msg[4] = AROS_LONG2LE(4);
    msg[5] = AROS_LONG2LE(VCCLOCK_SDHCI);
    msg[6] = 0;
    msg[7] = 0;
    if (MBoxCall((APTR)VCMB_BASE, VCMB_PROPCHAN, msg) == msg)
    {
        SDIOBase->sdio_ClockMax = AROS_LE2LONG(msg[6]);
        ok = (SDIOBase->sdio_ClockMax != 0);
    }

    FreeMem(raw, 8 * 4 + 16);
    return ok;
}

/* ----------------------------------------------------------------------- */
/* Polled command engine                                                   */

#define SDIO_CMD_ERR    (SDHCI_INT_ERROR | SDHCI_INT_TIMEOUT | SDHCI_INT_CRC | \
                         SDHCI_INT_END_BIT | SDHCI_INT_INDEX)

/* SDHCI "Transfer Complete" is bit 1. hardware/sdhc.h mislabels bit 2 as
 * SDHCI_INT_DATA_END (bit 2 is actually Block Gap Event, which never fires
 * for a normal transfer), so use the correct bit for the data-phase done
 * wait. (Not fixing the shared header - the sdcard driver tolerates it by
 * also exiting on byte-count.) */
#define SDIO_INT_XFER_DONE      (1 << 1)

/* Verbose per-command tracing - enabled only around the bring-up probe so
 * normal CMD52/CMD53 traffic stays quiet once the chip answers. */
static int sdio_trace = 0;

/* Trace the first few DATA-phase (CMD53 with payload) transfers so the first
 * large transfer (firmware upload) can be diagnosed without flooding. */
static int sdio_data_trace = 6;

/* One-shot full controller register snapshot for bring-up diagnosis. */
static void sdio_dumpregs(struct SDIOBase *SDIOBase, const char *when)
{
    D(bug("[SDIO] regs (%s):\n", when));
    D(bug("[SDIO]   BLKSZ=0x%04x BLKCNT=0x%04x ARG=0x%08x TM/CMD=0x%08x\n",
          sdio_rw(SDIOBase, SDHCI_BLOCK_SIZE), sdio_rw(SDIOBase, SDHCI_BLOCK_COUNT),
          sdio_rl(SDIOBase, SDHCI_ARGUMENT), sdio_rl(SDIOBase, SDHCI_TRANSFER_MODE)));
    D(bug("[SDIO]   PRESENT=0x%08x HOSTCTL=0x%02x PWR=0x%02x CLKCTL=0x%04x TMOUT=0x%02x\n",
          sdio_rl(SDIOBase, SDHCI_PRESENT_STATE), sdio_rb(SDIOBase, SDHCI_HOST_CONTROL),
          sdio_rb(SDIOBase, SDHCI_POWER_CONTROL), sdio_rw(SDIOBase, SDHCI_CLOCK_CONTROL),
          sdio_rb(SDIOBase, SDHCI_TIMEOUT_CONTROL)));
    D(bug("[SDIO]   INTSTAT=0x%08x INTEN=0x%08x SIGEN=0x%08x CTRL2=0x%08x CAPS=0x%08x\n",
          sdio_rl(SDIOBase, SDHCI_INT_STATUS), sdio_rl(SDIOBase, SDHCI_INT_ENABLE),
          sdio_rl(SDIOBase, SDHCI_SIGNAL_ENABLE), sdio_rl(SDIOBase, SDHCI_ACMD12_ERR),
          sdio_rl(SDIOBase, SDHCI_CAPABILITIES)));
    /* GPIO function-select banks: GPFSEL3 must still read 0x3ffff000 (GPIO
     * 34-39 = ALT3/SD1). If it changed, the VideoCore firmware re-muxed the
     * SDIO pins away from the Arasan and our commands never reach the chip. */
    D(bug("[SDIO]   GPFSEL3=0x%08x GPFSEL4=0x%08x GPFSEL5=0x%08x\n",
          AROS_LE2LONG(*(volatile ULONG *)GPFSEL3),
          AROS_LE2LONG(*(volatile ULONG *)GPFSEL4),
          AROS_LE2LONG(*(volatile ULONG *)GPFSEL5)));
}

/*
 * blksz > 0 selects multi-block mode (BLOCK_SIZE=blksz, BLOCK_COUNT=datalen/
 * blksz, TRANSMOD_MULTI) - required for CMD53 transfers larger than a single
 * byte-mode access on a given function. blksz == 0 is single byte-mode.
 */
static int sdio_command(struct SDIOBase *SDIOBase, UWORD cmd, ULONG arg,
                        ULONG rsptype, ULONG *resp,
                        APTR data, ULONG datalen, int dataread, ULONG blksz)
{
    ULONG inhibit = SDHCI_PS_CMD_INHIBIT | SDHCI_PS_DATA_INHIBIT;
    ULONG timeout, status;
    UWORD flags, transmode = 0;

    /* Wait for the controller to be ready (a slow in-flight data block at the
     * 400 kHz ident clock can hold DATA_INHIBIT for ~10 ms, so allow margin). */
    timeout = 200;
    while (sdio_rl(SDIOBase, SDHCI_PRESENT_STATE) & inhibit)
    {
        if (timeout-- == 0)
        {
            D(bug("[SDIO] CMD%u: inhibit stuck (PRESENT=0x%08x)\n", cmd,
                  sdio_rl(SDIOBase, SDHCI_PRESENT_STATE)));
            sdio_softreset(SDIOBase, SDHCI_RESET_CMD);
            sdio_softreset(SDIOBase, SDHCI_RESET_DATA);
            return -1;
        }
        sdio_udelay(SDIOBase, 1000);
    }

    sdio_wl(SDIOBase, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);

    if (!(rsptype & MMC_RSP_PRESENT))
        flags = SDHCI_CMD_RESP_NONE;
    else if (rsptype & MMC_RSP_136)
        flags = SDHCI_CMD_RESP_LONG;
    else if (rsptype & MMC_RSP_BUSY)
        flags = SDHCI_CMD_RESP_SHORT_BUSY;
    else
        flags = SDHCI_CMD_RESP_SHORT;

    if (rsptype & MMC_RSP_CRC)
        flags |= SDHCI_CMD_CRC;
    if (rsptype & MMC_RSP_OPCODE)
        flags |= SDHCI_CMD_INDEX;

    if (datalen > 0)
    {
        flags |= SDHCI_CMD_DATA;
        transmode = SDHCI_TRANSMOD_BLK_CNT_EN;
        if (dataread)
            transmode |= SDHCI_TRANSMOD_READ;

        sdio_wb(SDIOBase, SDHCI_TIMEOUT_CONTROL, SDHCI_TIMEOUT_MAX);
        if (blksz > 0)
        {
            /* Block mode: datalen/blksz blocks of blksz bytes */
            transmode |= SDHCI_TRANSMOD_MULTI;
            sdio_ww(SDIOBase, SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLCKSIZE(1, blksz));
            sdio_ww(SDIOBase, SDHCI_BLOCK_COUNT, datalen / blksz);
        }
        else
        {
            /* Byte mode: one "block" of datalen bytes */
            sdio_ww(SDIOBase, SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLCKSIZE(1, datalen));
            sdio_ww(SDIOBase, SDHCI_BLOCK_COUNT, 1);
        }
    }

    sdio_wl(SDIOBase, SDHCI_ARGUMENT, arg);

    /* BCM2835 Arasan: TRANSFER_MODE and COMMAND must be written together
     * as a single 32-bit access when a data phase is present. */
    if (datalen > 0)
        sdio_wl(SDIOBase, SDHCI_TRANSFER_MODE,
                (SDHCI_MAKE_CMD(cmd, flags) << 16) | transmode);
    else
        sdio_ww(SDIOBase, SDHCI_COMMAND, SDHCI_MAKE_CMD(cmd, flags));

    if (sdio_trace)
    {
        /* Capture the controller state the instant the command is issued,
         * BEFORE any (slow) serial output: if CMD_INHIBIT (PRESENT bit0) is
         * set here the controller accepted the command and is clocking it
         * out; if it never sets, the COMMAND write didn't start a transfer. */
        ULONG ps0 = sdio_rl(SDIOBase, SDHCI_PRESENT_STATE);
        ULONG is0 = sdio_rl(SDIOBase, SDHCI_INT_STATUS);
        D(bug("[SDIO] CMD%u arg=0x%08x flags=0x%02x issued: PRESENT=0x%08x INT=0x%08x ARGrb=0x%08x CMDrb=0x%04x\n",
              cmd, arg, flags, ps0, is0,
              sdio_rl(SDIOBase, SDHCI_ARGUMENT), sdio_rw(SDIOBase, SDHCI_COMMAND)));
    }

    {
    ULONG started = sdio_now(SDIOBase);
    timeout = 100000;
    while (!((status = sdio_rl(SDIOBase, SDHCI_INT_STATUS)) & (SDHCI_INT_RESPONSE | SDIO_CMD_ERR)))
    {
        if (timeout-- == 0)
        {
            D(bug("[SDIO] CMD%u: response timeout (status 0x%08x PRESENT=0x%08x)\n",
                  cmd, status, sdio_rl(SDIOBase, SDHCI_PRESENT_STATE)));
            sdio_wl(SDIOBase, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);
            sdio_softreset(SDIOBase, SDHCI_RESET_CMD);
            if (datalen > 0)            /* don't leave a pending CMD53 data phase
                                         * half-armed - the next CMD53 would see
                                         * stale DATA_AVAIL/SPACE_AVAIL */
                sdio_softreset(SDIOBase, SDHCI_RESET_DATA);
            return -1;
        }
        sdio_udelay(SDIOBase, 10);
    }

    if (status & SDIO_CMD_ERR)
    {
        D(bug("[SDIO] CMD%u: error status 0x%08x after %uus PRESENT=0x%08x\n",
              cmd, status, sdio_now(SDIOBase) - started,
              sdio_rl(SDIOBase, SDHCI_PRESENT_STATE)));
        sdio_wl(SDIOBase, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);
        sdio_softreset(SDIOBase, SDHCI_RESET_CMD);
        if (datalen > 0)                /* clear a half-armed data phase so the
                                         * next CMD53 doesn't desync on the FIFO */
            sdio_softreset(SDIOBase, SDHCI_RESET_DATA);
        return -1;
    }
    if (sdio_trace)
        D(bug("[SDIO] CMD%u: response after %uus (status 0x%08x)\n",
              cmd, sdio_now(SDIOBase) - started, status));
    }

    if (resp && (rsptype & MMC_RSP_PRESENT) && !(rsptype & MMC_RSP_136))
        *resp = sdio_rl(SDIOBase, SDHCI_RESPONSE);

    /* W1C the response bit */
    sdio_wl(SDIOBase, SDHCI_INT_STATUS, SDHCI_INT_RESPONSE);

    /* PIO data phase */
    if (datalen > 0)
    {
        ULONG intmask = dataread ? SDHCI_INT_DATA_AVAIL : SDHCI_INT_SPACE_AVAIL;
        ULONG bsize = (blksz > 0) ? blksz : datalen;    /* bytes per buffer-ready */
        ULONG nblk = (blksz > 0) ? (datalen / blksz) : 1;
        ULONG blk, off2 = 0;
        int dtr = (sdio_data_trace > 0 && datalen > 64);    /* skip tiny attach reads */

        if (dtr)
            D(bug("[SDIO] CMD%u data %s len=%u blksz=%u PRESENT=0x%08x INT=0x%08x\n",
                  cmd, dataread ? "rd" : "wr", datalen, blksz,
                  sdio_rl(SDIOBase, SDHCI_PRESENT_STATE),
                  sdio_rl(SDIOBase, SDHCI_INT_STATUS)));

        /* One buffer-ready handshake + PIO transfer per block. */
        for (blk = 0; blk < nblk; blk++)
        {
            ULONG words = (bsize + 3) >> 2, w;

            timeout = 100000;
            while (!(sdio_rl(SDIOBase, SDHCI_INT_STATUS) & (intmask | SDIO_CMD_ERR)))
            {
                if (timeout-- == 0)
                {
                    D(bug("[SDIO] CMD%u: data ready timeout blk %u (INT=0x%08x PRESENT=0x%08x)\n",
                          cmd, blk, sdio_rl(SDIOBase, SDHCI_INT_STATUS),
                          sdio_rl(SDIOBase, SDHCI_PRESENT_STATE)));
                    sdio_softreset(SDIOBase, SDHCI_RESET_DATA);
                    return -1;
                }
                sdio_udelay(SDIOBase, 10);
            }
            if (sdio_rl(SDIOBase, SDHCI_INT_STATUS) & SDIO_CMD_ERR)
            {
                D(bug("[SDIO] CMD%u: data error blk %u INT=0x%08x\n", cmd, blk,
                      sdio_rl(SDIOBase, SDHCI_INT_STATUS)));
                sdio_wl(SDIOBase, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);
                sdio_softreset(SDIOBase, SDHCI_RESET_DATA);
                return -1;
            }
            sdio_wl(SDIOBase, SDHCI_INT_STATUS, intmask);

            for (w = 0; w < words; w++)
            {
                ULONG n = bsize - (w << 2);
                if (n > 4)
                    n = 4;
                if (dataread)
                {
                    ULONG v = sdio_fifo_r(SDIOBase);
                    CopyMem(&v, (UBYTE *)data + off2 + (w << 2), n);
                }
                else
                {
                    ULONG v = 0;
                    CopyMem((UBYTE *)data + off2 + (w << 2), &v, n);
                    sdio_fifo_w(SDIOBase, v);
                }
            }
            off2 += bsize;
        }

        timeout = 100000;
        while (!(sdio_rl(SDIOBase, SDHCI_INT_STATUS) & (SDIO_INT_XFER_DONE | SDIO_CMD_ERR)))
        {
            if (timeout-- == 0)
            {
                D(bug("[SDIO] CMD%u: data end timeout (INT=0x%08x PRESENT=0x%08x)\n",
                      cmd, sdio_rl(SDIOBase, SDHCI_INT_STATUS),
                      sdio_rl(SDIOBase, SDHCI_PRESENT_STATE)));
                sdio_softreset(SDIOBase, SDHCI_RESET_DATA);
                return -1;
            }
            sdio_udelay(SDIOBase, 10);
        }
        sdio_wl(SDIOBase, SDHCI_INT_STATUS, SDIO_INT_XFER_DONE);

        if (dtr)
        {
            D(bug("[SDIO] CMD%u data done PRESENT=0x%08x INT=0x%08x\n", cmd,
                  sdio_rl(SDIOBase, SDHCI_PRESENT_STATE),
                  sdio_rl(SDIOBase, SDHCI_INT_STATUS)));
            sdio_data_trace--;
        }
    }

    return 0;
}

/* CMD52 - single register read/write */
static int sdio_rw_direct(struct SDIOBase *SDIOBase, int write, uint32_t func,
                          uint32_t addr, uint8_t in, uint8_t *out)
{
    ULONG arg, resp = 0;
    int err;

    arg = (func << SDIO_ARG_FUNC_SHIFT) | ((addr & SDIO_ARG_ADDR_MASK) << SDIO_ARG_ADDR_SHIFT);
    if (write)
        arg |= SDIO_ARG_RW_WRITE | (out ? SDIO_ARG_RAW : 0) | in;

    err = sdio_command(SDIOBase, SDIO_CMD_IO_RW_DIRECT, arg, MMC_RSP_R5, &resp, NULL, 0, 0, 0);
    if (err)
        return err;

    if ((resp >> 8) & SDIO_R5_ERRORS)
    {
        D(bug("[SDIO] CMD52 R5 flags 0x%02x (func %u addr 0x%05x)\n",
              (resp >> 8) & 0xff, func, addr));
        return -1;
    }

    if (out)
        *out = resp & 0xff;
    return 0;
}

/*
 * CMD53 - extended read/write. A single CMD53 moves at most 512 bytes (byte
 * mode) or 511 blocks (block mode), so larger transfers are split: whole
 * blocks go in block mode (required by e.g. the func1 backplane and func2
 * frame FIFO - a big byte-mode CMD53 returns R5 OUT_OF_RANGE), the sub-block
 * remainder in byte mode. For incr==0 (func2 frame FIFO, func1 4-byte window)
 * the address is fixed across chunks; for incr==1 (backplane RAM regions) it
 * advances by the bytes moved. Without the split, an event/data frame larger
 * than 512 bytes failed AND left the FIFO unread (desyncing every later read).
 */
static int sdio_rw_extended(struct SDIOBase *SDIOBase, int write, uint32_t func,
                            uint32_t addr, APTR buf, uint32_t len, uint32_t incr)
{
    ULONG blksz = (func < 8) ? SDIOBase->sdio_BlkSize[func] : 0;
    UBYTE *p = (UBYTE *)buf;

    if (len == 0)
        return -1;

    while (len > 0)
    {
        ULONG arg, resp = 0, chunk;
        int useblock, err;

        if (blksz > 0 && len >= blksz)
        {
            ULONG nblk = len / blksz;

            if (nblk > SDIO_ARG_COUNT_MASK)
                nblk = SDIO_ARG_COUNT_MASK;
            chunk = nblk * blksz;
            useblock = 1;
        }
        else
        {
            chunk = (len > 512) ? 512 : len;
            useblock = 0;
        }

        arg = (write ? SDIO_ARG_RW_WRITE : 0) | (func << SDIO_ARG_FUNC_SHIFT);
        if (incr)
            arg |= SDIO_ARG_OPCODE_INCR;
        arg |= (addr & SDIO_ARG_ADDR_MASK) << SDIO_ARG_ADDR_SHIFT;
        if (useblock)
            arg |= SDIO_ARG_BLOCKMODE | ((chunk / blksz) & SDIO_ARG_COUNT_MASK);
        else
            arg |= (chunk == 512 ? 0 : chunk) & SDIO_ARG_COUNT_MASK;  /* 0 => 512 */

        err = sdio_command(SDIOBase, SDIO_CMD_IO_RW_EXTENDED, arg, MMC_RSP_R5,
                           &resp, p, chunk, write ? 0 : 1, useblock ? blksz : 0);
        if (err)
            return err;

        if ((resp >> 8) & SDIO_R5_ERRORS)
        {
            D(bug("[SDIO] CMD53 R5 flags 0x%02x (func %u addr 0x%05x len %u %s)\n",
                  (resp >> 8) & 0xff, func, addr, chunk, write ? "wr" : "rd"));
            return -1;
        }

        p += chunk;
        len -= chunk;
        if (incr)
            addr += chunk;
    }

    return 0;
}

/*
 * Once the card is selected (CMD7), switch from the 1-bit / 400 kHz
 * identification setup to a 4-bit bus at 25 MHz (SDIO default speed). Both
 * card (CCCR) and host (Arasan HOST_CONTROL) must agree; CMD52 runs on the
 * single CMD line so the width change is safe to issue before flipping the
 * host. Hugely speeds up firmware upload + the datapath.
 *
 * NOTE: SDIO high-speed mode is deliberately NOT used. The BCM2835 Arasan
 * SDHCI HISPD bit is a known-broken quirk - setting it wedged the bus (the
 * next CMD52 timed out 0x18000). 25 MHz is the default-speed maximum, which
 * needs no HISPD bit, so we cap there.
 */
static void sdio_set_bus_speed(struct SDIOBase *SDIOBase)
{
    uint8_t v = 0, ctrl;

    /* 4-bit bus (CCCR Bus Interface Control 0x07, low 2 bits = 10b). */
    if (sdio_rw_direct(SDIOBase, 0, 0, SDIO_CCCR_BUS_CONTROL, 0, &v) == 0)
    {
        v = (v & ~SDIO_CCCR_BUS_WIDTH_MASK) | SDIO_CCCR_BUS_WIDTH_4BIT;
        if (sdio_rw_direct(SDIOBase, 1, 0, SDIO_CCCR_BUS_CONTROL, v, NULL) == 0)
        {
            ctrl = sdio_rb(SDIOBase, SDHCI_HOST_CONTROL) | SDHCI_HCTRL_4BITBUS;
            sdio_wb(SDIOBase, SDHCI_HOST_CONTROL, ctrl);
            D(bug("[SDIO] 4-bit bus enabled\n"));
        }
    }

    sdio_setclock(SDIOBase, SDIO_FULL_CLOCK);
}

/*
 * SDIO card identification: CMD5 OCR negotiation, CMD3 (relative address)
 * and CMD7 (select). Caller holds sdio_Sem (or is single-threaded init).
 */
/* Defined below in the card-interrupt section; installed by sdio_do_probe. */
static void sdio_irq_handler(void *data1, void *data2);

static int sdio_do_probe(struct SDIOBase *SDIOBase)
{
    ULONG ocr = 0, resp = 0;
    int retries;

    /* The chip is initialised once (CMD5/CMD3/CMD7, card selected). A second
     * full probe would send CMD0 + CMD5 to an already-selected card, which no
     * longer answers the init-phase CMD5 - so report the cached result. bwfm
     * calls SDIOProbe() before using the card; it must not re-initialise it. */
    if (SDIOBase->sdio_Present)
        return TRUE;

    SDIOBase->sdio_Present = FALSE;

    /* Lazy WiFi-chip bring-up: start the 32 kHz LPO and drive WL_REG_ON high,
     * then let the chip's power-on reset settle before probing. Done here (not
     * in sdio_init) so the chip stays unpowered until a client opens the bus.
     * Re-running on a probe retry just re-asserts the same lines (harmless). */
    sdio_wifi_clock(SDIOBase);
    sdio_wifi_power(SDIOBase);
    sdio_udelay(SDIOBase, 50000);

    sdio_dumpregs(SDIOBase, "pre-probe");

    /* Re-assert the SDIO pin mux: the firmware writes GPIO banks concurrently
     * (GPFSEL4 was seen changing between reads), so 34-39 may have been
     * re-routed since sdio_init muxed them. Harmless no-op if still ALT3. */
    sdio_gpio_mux(SDIOBase);

    sdio_trace = 1;             /* verbose per-command tracing for the probe */

    /*
     * SD/SDIO init preamble. The BCM43xx needs the standard SD reset +
     * interface-condition handshake before it will answer CMD5; sending
     * CMD5 cold (as a pure SDIO card would allow) just times out on this
     * chip. CMD0/CMD8 results are advisory here - proceed regardless.
     */
    sdio_command(SDIOBase, MMC_CMD_GO_IDLE_STATE, 0, MMC_RSP_NONE, NULL, NULL, 0, 0, 0);
    sdio_udelay(SDIOBase, 2000);
    sdio_command(SDIOBase, SD_CMD_SEND_IF_COND, 0x1AA, MMC_RSP_R7, &resp, NULL, 0, 0, 0);

    /* CMD5 with arg 0: read the I/O OCR */
    if (sdio_command(SDIOBase, SDIO_CMD_IO_SEND_OP_COND, 0, MMC_RSP_R4, &ocr, NULL, 0, 0, 0))
        return FALSE;

    SDIOBase->sdio_NumFunc = (ocr & SDIO_OCR_NUM_FUNC_MASK) >> SDIO_OCR_NUM_FUNC_SHIFT;

    /* CMD5 with the supported voltage window: wait for I/O ready */
    for (retries = 100; retries > 0; retries--)
    {
        if (sdio_command(SDIOBase, SDIO_CMD_IO_SEND_OP_COND,
                         ocr & SDIO_OCR_VOLTAGE_MASK, MMC_RSP_R4, &resp, NULL, 0, 0, 0))
            return FALSE;
        if (resp & SDIO_OCR_IOREADY)
            break;
        sdio_udelay(SDIOBase, 1000);
    }
    if (!(resp & SDIO_OCR_IOREADY))
        return FALSE;

    SDIOBase->sdio_OCR = resp;

    /* CMD3: ask the card to publish its relative address */
    if (sdio_command(SDIOBase, SDIO_CMD_SEND_RELATIVE_ADDR, 0, MMC_RSP_R6, &resp, NULL, 0, 0, 0))
        return FALSE;
    SDIOBase->sdio_RCA = (resp >> 16) & 0xffff;

    /* CMD7: select the card (move to command state) */
    if (sdio_command(SDIOBase, SDIO_CMD_SELECT_CARD,
                     SDIOBase->sdio_RCA << 16, MMC_RSP_R1b, &resp, NULL, 0, 0, 0))
        return FALSE;

    /* Card selected: leave identification mode for 4-bit + full clock. */
    sdio_set_bus_speed(SDIOBase);

    SDIOBase->sdio_Present = TRUE;
    sdio_trace = 0;

    D(bug("[SDIO] SDIO device present: OCR 0x%08x, %u function(s), RCA 0x%04x\n",
          SDIOBase->sdio_OCR, SDIOBase->sdio_NumFunc, SDIOBase->sdio_RCA));

    /* Install the card-interrupt handler now that a device answered (stays
     * masked out of INT_ENABLE until a consumer arms it via SDIOSetInterrupt).
     * The sdio_Present guard above ensures this runs only once. */
    if (SDIOBase->sdio_IRQHandle == NULL)
    {
        SDIOBase->sdio_IRQHandle = KrnAddIRQHandler(IRQ_VC_ARASANSDIO,
                                                    sdio_irq_handler, SDIOBase, NULL);
        D(bug("[SDIO] card IRQ handler %p on irq %u\n",
              SDIOBase->sdio_IRQHandle, IRQ_VC_ARASANSDIO));
    }
    return TRUE;
}

/* ----------------------------------------------------------------------- */
/* Card interrupt (SDIO DAT1) -> consumer task                             */

/*
 * INT_ENABLE value with the card interrupt suppressed. On the BCM2835 Arasan
 * the card interrupt reaches the ARM whenever it is SET in INT_STATUS, which
 * is gated by INT_ENABLE (0x34) - NOT by SIGNAL_ENABLE (0x38). So masking via
 * SIGNAL_ENABLE does nothing (a storm); we mask/arm the card int by toggling
 * it in INT_ENABLE with constant writes (no read-modify-write race with the
 * handler). The other (command/data) bits stay enabled for the polled path.
 */
#define SDIO_INTEN_BASE  (SDHCI_INT_ALL_MASK & ~SDHCI_INT_CARD_INT)

/*
 * Arasan IRQ handler. The only source we arm is the SDIO card interrupt (the
 * chip pulls DAT1 low when SDPCMD_INTSTATUS has an enabled bit). Mask it here
 * (drop CARD_INT from INT_ENABLE) so it cannot re-fire, and signal the bwfm RX
 * pump. The pump clears the chip's SDPCMD_INTSTATUS - which releases DAT1 -
 * drains its frames, then re-arms via SDIOAckInterrupt().
 */
static void sdio_irq_handler(void *data1, void *data2)
{
    struct SDIOBase *SDIOBase = (struct SDIOBase *)data1;

    if (sdio_rl(SDIOBase, SDHCI_INT_STATUS) & SDHCI_INT_CARD_INT)
    {
        sdio_wl(SDIOBase, SDHCI_INT_ENABLE, SDIO_INTEN_BASE);
        if (SDIOBase->sdio_IRQTask)
            Signal(SDIOBase->sdio_IRQTask, SDIOBase->sdio_IRQSig);
    }
}

/* ----------------------------------------------------------------------- */
/* Resource init                                                           */

static int sdio_init(struct SDIOBase *SDIOBase)
{
    D(bug("[SDIO] %s()\n", __PRETTY_FUNCTION__));

    KernelBase = OpenResource("kernel.resource");
    MBoxBase = OpenResource("mbox.resource");
    if (!KernelBase || !MBoxBase)
        return FALSE;

    if ((SDIOBase->sdio_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase)) == 0)
        return FALSE;

    InitSemaphore(&SDIOBase->sdio_Sem);
    SDIOBase->sdio_iobase = SDIOBase->sdio_periiobase + 0x300000;       /* ARASAN_BASE */
    SDIOBase->sdio_LastWrite = sdio_now(SDIOBase);

    if (!sdio_mbox_setup(SDIOBase))
    {
        D(bug("[SDIO] mailbox power/clock query failed\n"));
        return FALSE;
    }
    D(bug("[SDIO] Arasan base clock %u Hz\n", SDIOBase->sdio_ClockMax));

    sdio_gpio_mux(SDIOBase);
    sdio_gpio_pulls(SDIOBase);

    /*
     * The firmware boots the SD card on the Arasan (SD1) with GPIO48-53 left
     * at ALT3 - the SAME SD1 controller we just routed GPIO34-39 to for WiFi.
     * With two pad groups on one controller the boot card's idle-high CMD/DAT
     * lines mask the WiFi chip's responses, so every command times out. Park
     * GPIO48-53 as inputs so SD1 reaches only the WiFi pins. sdcard.device's
     * SDHOST driver (lower init priority, runs later) re-routes 48-53 to
     * ALT0/SD0 for the SD card. NOTE: assumes the SDHOST build (the Arasan is
     * not used for the SD card); see arch/.../sdcard mmakefile BCM2708BUILDFILES.
     */
    {
        volatile ULONG *fsel4 = (volatile ULONG *)GPFSEL4;
        volatile ULONG *fsel5 = (volatile ULONG *)GPFSEL5;

        *fsel4 = AROS_LONG2LE(AROS_LE2LONG(*fsel4) & ~((0x7u << 24) | (0x7u << 27)));
        *fsel5 = AROS_LONG2LE(AROS_LE2LONG(*fsel5) &
                 ~((0x7u << 0) | (0x7u << 3) | (0x7u << 6) | (0x7u << 9)));
        D(bug("[SDIO] GPIO48-53 -> input (free SD1 for WiFi) GPFSEL4=0x%08x GPFSEL5=0x%08x\n",
              AROS_LE2LONG(*fsel4), AROS_LE2LONG(*fsel5)));
    }

    /* WiFi-chip power (LPO clock + WL_REG_ON) is deferred to the first probe
     * (sdio_do_probe), so the chip stays unpowered until a client opens the
     * bus. The controller bring-up + GPIO arbitration above stays eager - it
     * must win the boot race against the SDHOST/sdcard driver for GPIO48-53. */

    sdio_softreset(SDIOBase, SDHCI_RESET_ALL);
    D(bug("[SDIO] host version 0x%04x caps 0x%08x\n",
          sdio_rw(SDIOBase, SDHCI_HOST_VERSION),
          sdio_rl(SDIOBase, SDHCI_CAPABILITIES)));

    /*
     * Clear CONTROL2 (host_control2 at 0x3C): the firmware uses this Arasan
     * controller for the boot SD card and may leave it in a UHS / 1.8V
     * signalling mode. The WiFi SDIO bus is 3.3V (1.8V is broken on the Pi),
     * so force plain 3.3V SDR12 by zeroing it. (WiFiPi does the same.)
     */
    D(bug("[SDIO] CONTROL2 (0x3C) was 0x%08x, clearing\n",
          sdio_rl(SDIOBase, SDHCI_ACMD12_ERR)));
    sdio_wl(SDIOBase, SDHCI_ACMD12_ERR, 0);     /* 0x3C = EMMC_CONTROL2 */

    /* Data timeout counter (TMCLK * 2^(7+13)) */
    sdio_wb(SDIOBase, SDHCI_TIMEOUT_CONTROL, 0x07);

    sdio_setpower(SDIOBase);
    sdio_setclock(SDIOBase, SDIO_IDENT_CLOCK);

    /* 1-bit bus, no high speed for identification */
    sdio_wb(SDIOBase, SDHCI_HOST_CONTROL,
            sdio_rb(SDIOBase, SDHCI_HOST_CONTROL) &
            ~(SDHCI_HCTRL_8BITBUS | SDHCI_HCTRL_4BITBUS | SDHCI_HCTRL_HISPD));

    /* Latch status bits for polling; no IRQ routed yet (card int masked out
     * of INT_ENABLE until a consumer arms it via SDIOSetInterrupt). */
    sdio_wl(SDIOBase, SDHCI_INT_ENABLE, SDIO_INTEN_BASE);
    sdio_wl(SDIOBase, SDHCI_SIGNAL_ENABLE, 0);

    D(bug("[SDIO] controller initialised (chip power + probe deferred to first open)\n"));

    return TRUE;
}

ADD2INITLIB(sdio_init, 0)

/* ----------------------------------------------------------------------- */
/* Public API                                                              */

AROS_LH0(int, SDIOProbe,
                struct SDIOBase *, SDIOBase, 1, Sdio)
{
    AROS_LIBFUNC_INIT

    int present;

    ObtainSemaphore(&SDIOBase->sdio_Sem);
    present = sdio_do_probe(SDIOBase);
    ReleaseSemaphore(&SDIOBase->sdio_Sem);

    return present;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(uint8_t, SDIOReadByte,
                AROS_LHA(uint32_t, func, D0),
                AROS_LHA(uint32_t, addr, D1),
                struct SDIOBase *, SDIOBase, 2, Sdio)
{
    AROS_LIBFUNC_INIT

    uint8_t val = 0;

    ObtainSemaphore(&SDIOBase->sdio_Sem);
    sdio_rw_direct(SDIOBase, 0, func, addr, 0, &val);
    ReleaseSemaphore(&SDIOBase->sdio_Sem);

    return val;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, SDIOWriteByte,
                AROS_LHA(uint32_t, func, D0),
                AROS_LHA(uint32_t, addr, D1),
                AROS_LHA(uint8_t, val, D2),
                struct SDIOBase *, SDIOBase, 3, Sdio)
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&SDIOBase->sdio_Sem);
    sdio_rw_direct(SDIOBase, 1, func, addr, val, NULL);
    ReleaseSemaphore(&SDIOBase->sdio_Sem);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(int, SDIOReadExt,
                AROS_LHA(uint32_t, func, D0),
                AROS_LHA(uint32_t, addr, D1),
                AROS_LHA(void *, buf, D2),
                AROS_LHA(uint32_t, len, D3),
                AROS_LHA(uint32_t, incr, D4),
                struct SDIOBase *, SDIOBase, 4, Sdio)
{
    AROS_LIBFUNC_INIT

    int err;

    ObtainSemaphore(&SDIOBase->sdio_Sem);
    err = sdio_rw_extended(SDIOBase, 0, func, addr, buf, len, incr);
    ReleaseSemaphore(&SDIOBase->sdio_Sem);

    return err;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(int, SDIOWriteExt,
                AROS_LHA(uint32_t, func, D0),
                AROS_LHA(uint32_t, addr, D1),
                AROS_LHA(void *, buf, D2),
                AROS_LHA(uint32_t, len, D3),
                AROS_LHA(uint32_t, incr, D4),
                struct SDIOBase *, SDIOBase, 5, Sdio)
{
    AROS_LIBFUNC_INIT

    int err;

    ObtainSemaphore(&SDIOBase->sdio_Sem);
    err = sdio_rw_extended(SDIOBase, 1, func, addr, buf, len, incr);
    ReleaseSemaphore(&SDIOBase->sdio_Sem);

    return err;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(int, SDIOEnableFunction,
                AROS_LHA(uint32_t, func, D0),
                struct SDIOBase *, SDIOBase, 6, Sdio)
{
    AROS_LIBFUNC_INIT

    uint8_t reg = 0;
    int retries, err;

    ObtainSemaphore(&SDIOBase->sdio_Sem);

    err = sdio_rw_direct(SDIOBase, 0, 0, SDIO_CCCR_IO_ENABLE, 0, &reg);
    if (!err)
    {
        reg |= (1 << func);
        err = sdio_rw_direct(SDIOBase, 1, 0, SDIO_CCCR_IO_ENABLE, reg, NULL);
    }

    for (retries = 50; !err && retries > 0; retries--)
    {
        uint8_t rdy = 0;
        if (sdio_rw_direct(SDIOBase, 0, 0, SDIO_CCCR_IO_READY, 0, &rdy))
        {
            err = -1;
            break;
        }
        if (rdy & (1 << func))
            break;
        sdio_udelay(SDIOBase, 1000);
    }

    ReleaseSemaphore(&SDIOBase->sdio_Sem);
    return err;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(int, SDIOSetBlockSize,
                AROS_LHA(uint32_t, func, D0),
                AROS_LHA(uint32_t, size, D1),
                struct SDIOBase *, SDIOBase, 7, Sdio)
{
    AROS_LIBFUNC_INIT

    int err;

    ObtainSemaphore(&SDIOBase->sdio_Sem);
    err = sdio_rw_direct(SDIOBase, 1, 0, SDIO_FBR_BASE(func) + SDIO_FBR_BLOCK_SIZE0,
                         size & 0xff, NULL);
    if (!err)
        err = sdio_rw_direct(SDIOBase, 1, 0, SDIO_FBR_BASE(func) + SDIO_FBR_BLOCK_SIZE1,
                             (size >> 8) & 0xff, NULL);
    if (!err && func < 8)
        SDIOBase->sdio_BlkSize[func] = size;        /* cache for CMD53 block mode */
    ReleaseSemaphore(&SDIOBase->sdio_Sem);

    return err;

    AROS_LIBFUNC_EXIT
}

/*
 * Cheap presence query: returns whether the card was enumerated+selected by
 * the resource's own init probe. Unlike SDIOProbe() this issues no bus
 * traffic, so a client (bwfm) can gate on it without re-running CMD0/CMD5 on
 * the already-selected card.
 */
AROS_LH0(int, SDIOIsPresent,
                struct SDIOBase *, SDIOBase, 8, Sdio)
{
    AROS_LIBFUNC_INIT

    return SDIOBase->sdio_Present;

    AROS_LIBFUNC_EXIT
}

/*
 * Register `task` to be signalled (with `sigmask`) whenever the SDIO card
 * interrupt fires. Enables the chip's interrupt master + function-1 source
 * (CCCR 0x04) and routes the SDHCI card interrupt to the ARM. The handler
 * masks the interrupt each time; the consumer re-arms it with
 * SDIOAckInterrupt() after draining its frames.
 */
AROS_LH2(int, SDIOSetInterrupt,
                AROS_LHA(struct Task *, task, A0),
                AROS_LHA(uint32_t, sigmask, D0),
                struct SDIOBase *, SDIOBase, 9, Sdio)
{
    AROS_LIBFUNC_INIT

    int err;

    SDIOBase->sdio_IRQTask = task;
    SDIOBase->sdio_IRQSig = sigmask;

    ObtainSemaphore(&SDIOBase->sdio_Sem);
    err = sdio_rw_direct(SDIOBase, 1, 0, SDIO_CCCR_INT_ENABLE,
                         SDIO_CCCR_IEN_MASTER | SDIO_CCCR_IEN_FUNC1, NULL);
    ReleaseSemaphore(&SDIOBase->sdio_Sem);

    /* Arm the card interrupt (gated by INT_ENABLE on this controller). */
    sdio_wl(SDIOBase, SDHCI_INT_ENABLE, SDIO_INTEN_BASE | SDHCI_INT_CARD_INT);

    D(bug("[SDIO] card interrupt armed (CCCR err %d)\n", err));
    return err;

    AROS_LIBFUNC_EXIT
}

/*
 * Re-arm the card interrupt after the consumer has drained pending frames.
 * (The handler masks it on each fire to avoid a level-triggered storm.)
 */
AROS_LH0(void, SDIOAckInterrupt,
                struct SDIOBase *, SDIOBase, 10, Sdio)
{
    AROS_LIBFUNC_INIT

    sdio_wl(SDIOBase, SDHCI_INT_ENABLE, SDIO_INTEN_BASE | SDHCI_INT_CARD_INT);

    AROS_LIBFUNC_EXIT
}
