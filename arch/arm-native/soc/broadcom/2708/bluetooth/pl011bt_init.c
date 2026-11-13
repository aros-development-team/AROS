/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pl011bt.resource -- the PL011 wired to the on-board Bluetooth radio.

    The BCM283x has two UARTs. On a Raspberry Pi 3 and later the firmware
    routes the PL011 to the on-board Bluetooth controller and leaves the AUX
    mini-UART on the GPIO header, which is where the serial console lives.
    This resource owns that PL011 -- the pin mux, the controller's 32.768 kHz
    low-power oscillator on GPCLK2, BT_REG_EN, the baud divisor and the
    receive interrupt -- and nothing above it.

    It is deliberately the transport and only the transport: H:4 framing is
    h4bthci.device beside this, the protocols are rom/bluetooth, and the
    Broadcom patchram upload is brcmbt.fwl.

    Resident initialisation stays side-effect free. It discovers the
    peripheral window and asks the firmware for the UART clock; the PL011 is
    not touched until a client claims the resource and configures it, so a
    machine that never opens Bluetooth is left exactly as the firmware set
    it up.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/macros.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/gpio.h>
#include <proto/mbox.h>
#include <proto/openfirmware.h>
#include <proto/pl011bt.h>

#include "pl011bt_private.h"

/*
 * The Bluetooth pins. GPIO 30-33 are CTS/RTS/TXD/RXD of the PL011 in ALT3,
 * and GPIO 43 carries GPCLK2 in ALT0, which the controller uses as its LPO.
 */
#define BT_GPIO_CTS             30
#define BT_GPIO_TXD             32
#define BT_GPIO_RXD             33
#define BT_GPIO_LPO             43

/* Mailbox tags for the firmware's GPIO expander, pins numbered from 128. */
#define FW_SET_GPIO_STATE       0x00038041
#define FW_SET_GPIO_CONFIG      0x00038043
#define FW_GPIO_BT_REG_EN       128
#define FW_GPIO_DIR_OUTPUT      1

/* Answer when the firmware will not say. The PL011 reference clock is 48 MHz
 * on every Pi that has an on-board radio. */
#define UART_CLOCK_FALLBACK_HZ  48000000

/* proto/kernel.h, proto/mbox.h and proto/gpio.h declare the conventional
 * library bases as extern APTR. Define them here so their inline calls use
 * the resources this one opened. */
APTR KernelBase;
APTR MBoxBase;
APTR GPIOBase;

static inline ULONG pl011bt_rd(IPTR addr)
{
    return AROS_LE2LONG(*(volatile ULONG *)addr);
}

static inline void pl011bt_wr(IPTR addr, ULONG value)
{
    *(volatile ULONG *)addr = AROS_LONG2LE(value);
}

/*
 * Route the four PL011 signals to the radio.
 *
 * Through gpio.resource rather than by hand: GPIOSetFunc() preserves the
 * other pins in the same GPFSEL, and on a BCM2712 it knows to go through the
 * RP1 instead. Both matter here -- GPFSEL3 also carries the WiFi SDIO bus on
 * GPIO 34-39, and clearing that from under sdio.resource is not recoverable.
 */
static void route_bluetooth_uart(struct PL011BTBase *PL011BTBase)
{
    unsigned int first = PL011BTBase->pl011bt_RTSCTS ? BT_GPIO_CTS : BT_GPIO_TXD;
    unsigned int pin;

    for (pin = first; pin <= BT_GPIO_RXD; pin++)
        GPIOSetFunc(pin, GPIO_FSEL_ALT3);

    D(bug("[PL011BT] GPIO %lu-%lu -> ALT3\n",
          (ULONG)first, (ULONG)BT_GPIO_RXD));
}

/*
 * Drive the controller's low-power oscillator: 32.768 kHz out of GPCLK2.
 *
 * Stop the generator by clearing ENAB *and nothing else*.
 *
 * Writing the password alone clears ENAB, but in the same store also drives
 * SRC to 0 (GND) and MASH to 0. The BCM283x peripherals datasheet is explicit
 * that the source and the divisor must not change while the generator runs:
 * clear ENAB, wait for BUSY to fall, and only then reprogram. Changing SRC in
 * the stop write leaves BUSY set and the wait below never completes.
 *
 * On a Pi the firmware has already started GPCLK2 for the radio, so the bad
 * write lands on a running generator every time. It passes anywhere the
 * register is not modelled and BUSY reads back 0.
 */
static LONG setup_lpo_clock(struct PL011BTBase *PL011BTBase)
{
    ULONG wait = PL011BT_WAIT_LIMIT;
    ULONG ctl;

    GPIOSetFunc(BT_GPIO_LPO, GPIO_FSEL_ALT0);

    ctl = pl011bt_rd(CM_GP2CTL) & (CM_SRC_MASK | CM_MASH_MASK);
    pl011bt_wr(CM_GP2CTL, CM_PASSWORD | ctl);
    while ((pl011bt_rd(CM_GP2CTL) & CM_BUSY) && --wait != 0)
        ;
    if (wait == 0)
        return PL011BT_ERR_TIMEOUT;

    /* 19.2 MHz / 585.9375 = 32768 Hz. The fractional field is in 1/4096. */
    pl011bt_wr(CM_GP2DIV, CM_PASSWORD | (585 << 12) | 3840);
    pl011bt_wr(CM_GP2CTL,
               CM_PASSWORD | CM_MASH(1) | CM_SRC_OSC | CM_ENAB);

    D(bug("[PL011BT] GPCLK2 -> 32768 Hz on GPIO %lu\n", (ULONG)BT_GPIO_LPO));
    return PL011BT_OK;
}

/*
 * Expander pins are reachable only through the mailbox.
 * Caller holds pl011bt_Sem: the message buffer is shared.
 */
static LONG firmware_gpio_set(struct PL011BTBase *PL011BTBase, ULONG gpio,
                              ULONG enabled)
{
    ULONG *msg = PL011BTBase->pl011bt_MBoxMsg;
    LONG result = PL011BT_ERR_UNAVAILABLE;

    if (MBoxBase == NULL || msg == NULL)
    {
        bug("[PL011BT] power: mbox.resource unavailable\n");
        return result;
    }

    msg[0] = AROS_LONG2LE(12 * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(FW_SET_GPIO_CONFIG);
    msg[3] = AROS_LONG2LE(24);
    msg[4] = 0;
    msg[5] = AROS_LONG2LE(gpio);
    msg[6] = AROS_LONG2LE(FW_GPIO_DIR_OUTPUT);
    msg[7] = 0;
    msg[8] = 0;
    msg[9] = 0;
    msg[10] = AROS_LONG2LE(enabled != 0);
    msg[11] = 0;
    if (MBoxCall((APTR)(IPTR)VCMB_BASE, VCMB_PROPCHAN, msg) == msg &&
        (AROS_LE2LONG(msg[1]) & VCTAG_RESP))
        result = PL011BT_OK;

    if (result == PL011BT_OK)
    {
        msg[0] = AROS_LONG2LE(8 * 4);
        msg[1] = AROS_LONG2LE(VCTAG_REQ);
        msg[2] = AROS_LONG2LE(FW_SET_GPIO_STATE);
        msg[3] = AROS_LONG2LE(8);
        msg[4] = 0;
        msg[5] = AROS_LONG2LE(gpio);
        msg[6] = AROS_LONG2LE(enabled != 0);
        msg[7] = 0;
        if (MBoxCall((APTR)(IPTR)VCMB_BASE, VCMB_PROPCHAN, msg) != msg ||
            !(AROS_LE2LONG(msg[1]) & VCTAG_RESP))
            result = PL011BT_ERR_UNAVAILABLE;
    }

    D(bug("[PL011BT] power: GPIO %lu -> %lu, result %ld\n",
          (ULONG)gpio, (ULONG)(enabled != 0), (LONG)result));
    return result;
}

static ULONG query_uart_clock(struct PL011BTBase *PL011BTBase)
{
    ULONG *msg = PL011BTBase->pl011bt_MBoxMsg;

    if (MBoxBase == NULL || msg == NULL)
        return UART_CLOCK_FALLBACK_HZ;

    msg[0] = AROS_LONG2LE(8 * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(VCTAG_GETCLKRATE);
    msg[3] = AROS_LONG2LE(8);
    msg[4] = AROS_LONG2LE(4);
    msg[5] = AROS_LONG2LE(VCCLOCK_UART);
    msg[6] = 0;
    msg[7] = 0;

    if (MBoxCall((APTR)(IPTR)VCMB_BASE, VCMB_PROPCHAN, msg) != msg)
        return UART_CLOCK_FALLBACK_HZ;
    if (AROS_LE2LONG(msg[6]) == 0)
        return UART_CLOCK_FALLBACK_HZ;
    return AROS_LE2LONG(msg[6]);
}

/*
 * Drain the FIFO into the ring. Runs in interrupt context.
 *
 * Plain void(void *, void *), which is what KrnAddIRQHandler() calls. Writing
 * it as AROS_INTH1 produces the struct Interrupt server convention instead,
 * and being called as a plain pointer through that mismatch corrupts
 * registers.
 *
 * Everything here is a store to MMIO or to the ring: no allocation, no lock,
 * no call back into exec. The reader synchronises on the head alone, which
 * this is the only writer of.
 */
static void pl011bt_rx_handler(void *data, void *unused)
{
    struct PL011BTBase *PL011BTBase = data;
    ULONG head;

    (void)unused;
    head = PL011BTBase->pl011bt_RXHead;

    while (!(pl011bt_rd(PL011_0_BASE + PL011_FR) & PL011_FR_RXFE))
    {
        ULONG dr = pl011bt_rd(PL011_0_BASE + PL011_DR);
        ULONG next = (head + 1) & (PL011BT_RX_RING - 1);

        if ((dr & PL011_DR_ERR) != 0 && PL011BTBase->pl011bt_RXErrors < 8)
        {
            PL011BTBase->pl011bt_RXErrors++;
            bug("[PL011BT] rx status 0x%lx%s\n", (ULONG)((dr >> 8) & 0xF),
                (dr & PL011_DR_OE) ? " OVERRUN" : "");
        }
        if (next == PL011BTBase->pl011bt_RXTail)
        {
            /* The reader is too far behind. Dropping here is still better
             * than letting the FIFO overrun, because it is counted. */
            PL011BTBase->pl011bt_RXDropped++;
            break;
        }
        PL011BTBase->pl011bt_RXRing[head] = (UBYTE)(dr & 0xFF);
        head = next;
    }
    PL011BTBase->pl011bt_RXHead = head;

    /* Acknowledge receive and receive-timeout; overrun goes with them so a
     * single lost byte does not latch the condition forever. */
    pl011bt_wr(PL011_0_BASE + PL011_ICR,
               PL011_INT_RX | PL011_INT_RT | PL011_INT_OE);
}

/*
 * serial0 is the console. disable-bt and miniuart-bt point it at the PL011
 * (serial@7e201000), and then the PL011 is not ours.
 */
static BOOL pl011bt_is_console(void)
{
    static const char pl011[] = "201000";
    APTR OpenFirmwareBase = OpenResource("openfirmware.resource");
    void *key, *prop;
    const char *s;
    ULONG len, i;

    if (!OpenFirmwareBase || !(key = OF_OpenKey("/aliases")) ||
        !(prop = OF_FindProperty(key, "serial0")))
        return FALSE;

    /* The length counts the terminating NUL. */
    len = OF_GetPropLen(prop);
    if (len < sizeof(pl011))
        return FALSE;
    s = (const char *)OF_GetPropValue(prop) + len - sizeof(pl011);
    for (i = 0; i < sizeof(pl011) - 1; i++)
        if (s[i] != pl011[i])
            return FALSE;

    return TRUE;
}

/*
 * CTS/RTS are wired to GPIO 30/31 when the live uart0_pins list them
 * (3B+, Zero 2 W), not on a Pi 3B. Default: wired.
 */
static BOOL pl011bt_has_rtscts(void)
{
    APTR OpenFirmwareBase = OpenResource("openfirmware.resource");
    void *key, *prop;
    const ULONG *pins;
    ULONG n, i;

    if (!OpenFirmwareBase ||
        !(key = OF_OpenKey("/soc/gpio@7e200000/uart0_pins")) ||
        !(prop = OF_FindProperty(key, "brcm,pins")))
        return TRUE;

    pins = (const ULONG *)OF_GetPropValue(prop);
    n = OF_GetPropLen(prop) / sizeof(ULONG);
    for (i = 0; i < n; i++)
        if (AROS_BE2LONG(pins[i]) == BT_GPIO_CTS)
            return TRUE;

    return FALSE;
}

/*
 * BT_REG_EN from the radio's shutdown-gpios: the expander on a Pi 3B/3B+,
 * SoC GPIO 42 on a Zero 2 W. Default: expander pin 0.
 */
static void pl011bt_find_power(struct PL011BTBase *PL011BTBase)
{
    APTR OpenFirmwareBase = OpenResource("openfirmware.resource");
    void *key, *prop, *gpio, *phandle;
    const ULONG *cell;

    PL011BTBase->pl011bt_PowerPin = FW_GPIO_BT_REG_EN;
    PL011BTBase->pl011bt_PowerSoC = FALSE;
    PL011BTBase->pl011bt_PowerLow = FALSE;

    if (!OpenFirmwareBase ||
        !(key = OF_OpenKey("/soc/serial@7e201000/bluetooth")) ||
        !(prop = OF_FindProperty(key, "shutdown-gpios")) ||
        OF_GetPropLen(prop) < 3 * sizeof(ULONG))
        return;

    cell = (const ULONG *)OF_GetPropValue(prop);
    gpio = OF_OpenKey("/soc/gpio@7e200000");
    phandle = gpio ? OF_FindProperty(gpio, "phandle") : NULL;

    /* Both big-endian: compare as is. */
    if (phandle && OF_GetPropLen(phandle) == sizeof(ULONG) &&
        *(const ULONG *)OF_GetPropValue(phandle) == cell[0])
    {
        PL011BTBase->pl011bt_PowerSoC = TRUE;
        PL011BTBase->pl011bt_PowerPin = AROS_BE2LONG(cell[1]);
    }
    else
        PL011BTBase->pl011bt_PowerPin = FW_GPIO_BT_REG_EN + AROS_BE2LONG(cell[1]);
    PL011BTBase->pl011bt_PowerLow = (AROS_BE2LONG(cell[2]) & 1) != 0;
}

static int pl011bt_init(struct PL011BTBase *PL011BTBase)
{
    IPTR periiobase;

    InitSemaphore(&PL011BTBase->pl011bt_Sem);
    PL011BTBase->pl011bt_Owner = NULL;
    PL011BTBase->pl011bt_Caps = 0;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase == NULL)
        return FALSE;

    /*
     * Held as an IPTR for the test, because the BCM2712 window does not fit
     * in the unsigned int the resource stores -- truncating first would let
     * a 64-bit base alias onto one of the windows accepted below.
     */
    periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    /*
     * Bind to the SoCs this has been run on, by naming them rather than by
     * excluding the others.
     *
     * The arrangement this drives -- PL011 on GPIO 30-33 to the radio, the
     * console pushed onto the AUX mini-UART, the LPO on GPCLK2, BT_REG_EN on
     * the expander or a SoC GPIO -- is the BCM2835/6/7 one, and IRQ_VC_UART
     * below is the legacy interrupt controller's numbering. A BCM2711 wires
     * its radio the same way but presents the GPU interrupts through the GIC
     * at BCM2711_GPUIRQ_OFFSET, and a BCM2712 does not use this UART for
     * Bluetooth at all. Neither has been tested, so the resource stands down
     * there: absent is a state h4bthci.device already handles, present and
     * wrong is not.
     */
    if (periiobase != BCM2835_PERIPHYSBASE &&
        periiobase != BCM2836_PERIPHYSBASE)
    {
        D(bug("[PL011BT] peripheral window 0x%p is not a BCM283x, standing"
              " down\n", (void *)periiobase));
        return FALSE;
    }
    PL011BTBase->pl011bt_periiobase = (unsigned int)periiobase;

    if (pl011bt_is_console())
    {
        bug("[PL011BT] the PL011 is serial0 (the console), standing down\n");
        return FALSE;
    }

    GPIOBase = OpenResource("gpio.resource");
    if (GPIOBase == NULL)
    {
        bug("[PL011BT] gpio.resource unavailable, cannot route the UART\n");
        return FALSE;
    }
    MBoxBase = OpenResource("mbox.resource");

    PL011BTBase->pl011bt_MBoxRaw = AllocMem(64 + (MBOX_MSG_ALIGN - 1),
                                            MEMF_PUBLIC | MEMF_CLEAR);
    if (PL011BTBase->pl011bt_MBoxRaw == NULL)
        return FALSE;
    PL011BTBase->pl011bt_MBoxMsg = (ULONG *)
        (((IPTR)PL011BTBase->pl011bt_MBoxRaw + (MBOX_MSG_ALIGN - 1))
         & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    PL011BTBase->pl011bt_ClockHz = query_uart_clock(PL011BTBase);
    PL011BTBase->pl011bt_RTSCTS = pl011bt_has_rtscts();
    pl011bt_find_power(PL011BTBase);
    PL011BTBase->pl011bt_Caps = PL011BT_CAP_PRESENT |
        PL011BT_CAP_BAUD_CHANGE | PL011BT_CAP_POWER_CONTROL;

    bug("[PL011BT] ready: PL011=0x%08lx clock=%lu mbox=%s rts/cts=%s"
        " power=%s%lu\n",
        (ULONG)PL011_0_BASE, (ULONG)PL011BTBase->pl011bt_ClockHz,
        MBoxBase != NULL ? "yes" : "no",
        PL011BTBase->pl011bt_RTSCTS ? "yes" : "no",
        PL011BTBase->pl011bt_PowerSoC ? "gpio" : "fw",
        (ULONG)PL011BTBase->pl011bt_PowerPin);

    return TRUE;
}

/*****************************************************************************
    NAME
        unsigned int PL011BTGetAPIVersion(

    SYNOPSIS
        )

    FUNCTION
        Return the interface version of pl011bt.resource.

    RESULT
        PL011BT_API_VERSION, currently 1.

    NOTES
        Obtain the resource with OpenResource("pl011bt.resource") before
        calling its functions. A version result describes the software
        interface; it does not establish that the controller is powered,
        configured or able to receive data. No ownership claim is required.

    SEE ALSO
        PL011BTGetCapabilities(), PL011BTClaim()

*****************************************************************************/
AROS_LH0(unsigned int, PL011BTGetAPIVersion,
    struct PL011BTBase *, PL011BTBase, 1, Pl011bt)
{
    AROS_LIBFUNC_INIT

    return PL011BT_API_VERSION;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        unsigned long PL011BTGetCapabilities(

    SYNOPSIS
        )

    FUNCTION
        Return the resource's transport capability flags.

    RESULT
        Bitwise combination of PL011BT_CAP_* values:
            PRESENT - The resource initialized for a supported peripheral window.
            RX_INTERRUPT - A receive interrupt handler has been registered.
            TX_INTERRUPT - Reserved; not set by this implementation.
            BAUD_CHANGE - UART divisor configuration is implemented.
            POWER_CONTROL - Controller power-line control is implemented.

    NOTES
        No ownership claim is required. RX_INTERRUPT is added after successful
        handler registration during PL011BTConfigure(). It remains set after
        PL011BTRelease() and does not mean receive interrupts are currently
        enabled. The flags do not report controller power or UART readiness.

        POWER_CONTROL does not guarantee that a firmware GPIO request can
        succeed. Check the result of PL011BTSetPower(). RTS/CTS wiring is not
        reported by this mask.

    SEE ALSO
        PL011BTGetAPIVersion(), PL011BTConfigure(), PL011BTSetPower()

*****************************************************************************/
AROS_LH0(unsigned long, PL011BTGetCapabilities,
    struct PL011BTBase *, PL011BTBase, 2, Pl011bt)
{
    AROS_LIBFUNC_INIT

    return PL011BTBase->pl011bt_Caps;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        long PL011BTClaim(

    SYNOPSIS
        void *owner)

    FUNCTION
        Reserve the Bluetooth UART transport for an owner token.

    INPUTS
        owner - Non-NULL token identifying the client. The pointer is compared
            by identity and retained until release; it is not dereferenced.
            Keep the token unique for the duration of the claim.

    RESULT
        PL011BT_OK on success, PL011BT_ERR_ARGUMENT for NULL, or
        PL011BT_ERR_BUSY if any owner already holds the resource, including
        the same token. Claims are not recursive or reference-counted.

    NOTES
        Call in task context. May wait for the resource semaphore, but does
        not wait for an existing owner to release its claim. Claiming does
        not power or configure the controller and does not clear receive data.

        Use the same token for subsequent operations. Configure the UART
        before data transfer and arrange a single reader and a single writer.
        The token does not serialize calls made by the same client. Release
        the claim with PL011BTRelease() after transfers have stopped.

    SEE ALSO
        PL011BTConfigure(), PL011BTSetPower(), PL011BTRelease()

*****************************************************************************/
AROS_LH1(long, PL011BTClaim,
    AROS_LHA(void *, owner, A0),
    struct PL011BTBase *, PL011BTBase, 3, Pl011bt)
{
    AROS_LIBFUNC_INIT

    LONG result = PL011BT_OK;

    if (owner == NULL)
        return PL011BT_ERR_ARGUMENT;

    ObtainSemaphore(&PL011BTBase->pl011bt_Sem);
    if (PL011BTBase->pl011bt_Owner != NULL)
        result = PL011BT_ERR_BUSY;
    else
        PL011BTBase->pl011bt_Owner = owner;
    ReleaseSemaphore(&PL011BTBase->pl011bt_Sem);

    return result;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        void PL011BTRelease(

    SYNOPSIS
        void *owner)

    FUNCTION
        Disable the UART and release a matching ownership claim.

    INPUTS
        owner - Token used for a successful PL011BTClaim(). NULL or a token
            that does not match the current owner has no effect.

    NOTES
        Call in task context after all client reads and writes have stopped.
        May wait for the resource semaphore. Disables UART interrupts and the
        UART, then clears the owner and saved configuration. It does not wait
        for bytes already accepted for transmission to reach the controller.

        Does not power down the controller, stop its low-power clock, restore
        pin routing, unregister the interrupt handler or clear the receive
        ring. If power-down is required, call PL011BTSetPower(owner, 0) before
        release. No completion status is returned.

    SEE ALSO
        PL011BTClaim(), PL011BTSetPower(), PL011BTWrite()

*****************************************************************************/
AROS_LH1(void, PL011BTRelease,
    AROS_LHA(void *, owner, A0),
    struct PL011BTBase *, PL011BTBase, 4, Pl011bt)
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&PL011BTBase->pl011bt_Sem);
    if (owner != NULL && PL011BTBase->pl011bt_Owner == owner)
    {
        pl011bt_wr(PL011_0_BASE + PL011_IMSC, 0);
        pl011bt_wr(PL011_0_BASE + PL011_CR, 0);
        PL011BTBase->pl011bt_Owner = NULL;
        PL011BTBase->pl011bt_Baud = 0;
        PL011BTBase->pl011bt_ConfigFlags = 0;
    }
    ReleaseSemaphore(&PL011BTBase->pl011bt_Sem);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        long PL011BTConfigure(

    SYNOPSIS
        void *owner, unsigned long baud, unsigned long flags)

    FUNCTION
        Configure the Bluetooth UART and reset its receive path.

    INPUTS
        owner - Non-NULL token holding the resource claim.
        baud - Nonzero requested bit rate. The rounded clock divisor must
            have an integer part from 1 through 65535.
        flags - PL011BT_CONFIG_RTS_CTS requests hardware flow control. It is
            enabled only when the resource considers those signals wired.
            Other bits have no effect on hardware configuration.

    RESULT
        PL011BT_OK after UART setup; PL011BT_ERR_ARGUMENT for NULL owner,
        zero baud or an unusable divisor; PL011BT_ERR_NOT_OWNER for a token
        without the claim; PL011BT_ERR_TIMEOUT if clock or UART busy polling
        exhausts its iteration limit. Success does not confirm receive IRQ
        registration; check PL011BT_CAP_RX_INTERRUPT afterward.

    NOTES
        Call in task context with client transfers stopped. May wait for the
        resource semaphore. Sets up the controller's 32.768 kHz clock and pin
        routing, then enables 8-bit UART data, no parity, one stop bit and
        FIFOs. Does not change controller power or negotiate its baud rate.
        Coordinate baud changes and controller startup with the higher layer.

        Reconfiguration discards queued receive data and resets the overflow
        count. The hardware FIFO drain is bounded; remaining input is not
        reported. RTS/CTS requests may be silently ignored when wiring is
        absent. Unknown flag bits are retained but otherwise ignored.

        Clock and UART busy waits are bounded by iteration counts, not elapsed
        time. Failed calls can leave clock, pins or UART state changed. An
        invalid divisor is detected after disabling the UART and clearing the
        receive ring. There is no rollback to the previous configuration.

    BUGS
        Receive-handler registration failure still returns PL011BT_OK.
        PL011BTRead() has no FIFO polling fallback and cannot receive new
        bytes without that handler. Divisor arithmetic is not checked for
        overflow before range validation.

    SEE ALSO
        PL011BTClaim(), PL011BTGetCapabilities(), PL011BTSetPower(),
        PL011BTRead(), PL011BTWrite()

*****************************************************************************/
AROS_LH3(long, PL011BTConfigure,
    AROS_LHA(void *, owner, A0),
    AROS_LHA(unsigned long, baud, D0),
    AROS_LHA(unsigned long, flags, D1),
    struct PL011BTBase *, PL011BTBase, 5, Pl011bt)
{
    AROS_LIBFUNC_INIT

    ULONG divisor, integer, fraction, control;
    ULONG wait = PL011BT_WAIT_LIMIT;
    ULONG guard;

    if (owner == NULL || baud == 0)
        return PL011BT_ERR_ARGUMENT;

    ObtainSemaphore(&PL011BTBase->pl011bt_Sem);
    if (PL011BTBase->pl011bt_Owner != owner)
    {
        ReleaseSemaphore(&PL011BTBase->pl011bt_Sem);
        return PL011BT_ERR_NOT_OWNER;
    }

    if (setup_lpo_clock(PL011BTBase) != PL011BT_OK)
    {
        bug("[PL011BT] configure: LPO clock timeout\n");
        ReleaseSemaphore(&PL011BTBase->pl011bt_Sem);
        return PL011BT_ERR_TIMEOUT;
    }
    route_bluetooth_uart(PL011BTBase);

    pl011bt_wr(PL011_0_BASE + PL011_CR, 0);
    while ((pl011bt_rd(PL011_0_BASE + PL011_FR) & PL011_FR_BUSY) && --wait != 0)
        ;
    if (wait == 0)
    {
        bug("[PL011BT] configure: PL011 busy timeout\n");
        ReleaseSemaphore(&PL011BTBase->pl011bt_Sem);
        return PL011BT_ERR_TIMEOUT;
    }

    pl011bt_wr(PL011_0_BASE + PL011_LCRH, 0);
    pl011bt_wr(PL011_0_BASE + PL011_IMSC, 0);
    pl011bt_wr(PL011_0_BASE + PL011_ICR, PL011_INT_ALL);

    /*
     * Start from an empty receive path, not from whatever the line held.
     *
     * Clearing the interrupt status says nothing about the FIFO's contents.
     * Powering the controller through BT_REG_EN and reprogramming the baud
     * divisor both put transitions on the wire, and whatever the receiver
     * made of those sits in the FIFO waiting to be read as if it were HCI. On
     * hardware that shows up as an OVERRUN before a single command has been
     * sent, followed by the H:4 framer reporting unknown packet types -- it
     * is not desynchronised by a lost reply, it never had synchronisation.
     */
    guard = PL011BT_RX_RING;
    while (guard-- &&
           !(pl011bt_rd(PL011_0_BASE + PL011_FR) & PL011_FR_RXFE))
        (void)pl011bt_rd(PL011_0_BASE + PL011_DR);

    PL011BTBase->pl011bt_RXHead = 0;
    PL011BTBase->pl011bt_RXTail = 0;
    PL011BTBase->pl011bt_RXDropped = 0;
    pl011bt_wr(PL011_0_BASE + PL011_ICR, PL011_INT_ALL);

    divisor = (PL011BTBase->pl011bt_ClockHz * 4 + baud / 2) / baud;
    integer = divisor >> 6;
    fraction = divisor & 0x3F;
    if (integer == 0 || integer > 0xFFFF)
    {
        ReleaseSemaphore(&PL011BTBase->pl011bt_Sem);
        return PL011BT_ERR_ARGUMENT;
    }
    pl011bt_wr(PL011_0_BASE + PL011_IBRD, integer);
    pl011bt_wr(PL011_0_BASE + PL011_FBRD, fraction);
    pl011bt_wr(PL011_0_BASE + PL011_LCRH,
               PL011_LCRH_WLEN8 | PL011_LCRH_FEN);

    control = PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE;
    /* Without the wires CTS floats and CTSEN can stall TX. */
    if ((flags & PL011BT_CONFIG_RTS_CTS) && PL011BTBase->pl011bt_RTSCTS)
        control |= PL011_CR_RTSEN | PL011_CR_CTSEN;
    pl011bt_wr(PL011_0_BASE + PL011_CR, control);

    /*
     * Arm receive interrupts once, after the port is configured.
     *
     * Order matters: the watermark and the mask are cleared by the LCRH write
     * above, and enabling the interrupt before UARTEN would deliver on a port
     * that is not receiving. Registration is idempotent because a second
     * Configure by the same owner is legitimate -- a baud change, for one.
     */
    pl011bt_wr(PL011_0_BASE + PL011_IFLS, PL011_IFLS_RX18);
    if (PL011BTBase->pl011bt_RXIRQ == NULL)
    {
        PL011BTBase->pl011bt_RXIRQ = KrnAddIRQHandler(IRQ_VC_UART,
            pl011bt_rx_handler, PL011BTBase, SysBase);
        if (PL011BTBase->pl011bt_RXIRQ != NULL)
        {
            PL011BTBase->pl011bt_Caps |= PL011BT_CAP_RX_INTERRUPT;
            D(bug("[PL011BT] rx interrupt armed on irq %lu\n",
                  (ULONG)IRQ_VC_UART));
        }
        else
            bug("[PL011BT] KrnAddIRQHandler(%lu) failed -- receive stays"
                " polled\n", (ULONG)IRQ_VC_UART);
    }
    pl011bt_wr(PL011_0_BASE + PL011_ICR, PL011_INT_ALL);
    if (PL011BTBase->pl011bt_RXIRQ != NULL)
        pl011bt_wr(PL011_0_BASE + PL011_IMSC, PL011_INT_RX | PL011_INT_RT);

    PL011BTBase->pl011bt_Baud = baud;
    PL011BTBase->pl011bt_ConfigFlags = flags;

    D(bug("[PL011BT] configure: baud=%lu clock=%lu divisor=%lu/%lu"
          " flags=0x%lx\n",
          (ULONG)baud, (ULONG)PL011BTBase->pl011bt_ClockHz,
          (ULONG)integer, (ULONG)fraction, (ULONG)flags));

    ReleaseSemaphore(&PL011BTBase->pl011bt_Sem);
    return PL011BT_OK;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        long PL011BTSetPower(

    SYNOPSIS
        void *owner, unsigned long enabled)

    FUNCTION
        Set the Bluetooth controller's power-enable line.

    INPUTS
        owner - Non-NULL token holding the resource claim.
        enabled - Zero to disable the controller; nonzero to enable it.

    RESULT
        PL011BT_OK after issuing the GPIO change, PL011BT_ERR_ARGUMENT for
        NULL owner, PL011BT_ERR_NOT_OWNER for a token without the claim, or
        PL011BT_ERR_UNAVAILABLE when the firmware GPIO path cannot complete
        its request. Success does not confirm controller readiness.

    NOTES
        Call in task context. May wait for the resource semaphore. Uses the
        power pin and polarity determined during initialization, through a
        SoC GPIO or firmware mailbox. The SoC path provides no hardware
        acknowledgement; firmware response checks do not verify pin readback.

        Power changes do not reconfigure the UART, clear receive data or
        release ownership. Disabling power can interrupt controller activity
        and discard its volatile state. The caller must coordinate transfers,
        startup delays, reset and any firmware reload required by the radio.
        Repeated calls issue the requested GPIO operations again.

    BUGS
        The firmware path inherits mailbox waits without an overall deadline
        and can block while holding the resource semaphore. A failed request
        can leave GPIO configuration partially changed.

    SEE ALSO
        PL011BTClaim(), PL011BTConfigure(), PL011BTRelease()

*****************************************************************************/
AROS_LH2(long, PL011BTSetPower,
    AROS_LHA(void *, owner, A0),
    AROS_LHA(unsigned long, enabled, D0),
    struct PL011BTBase *, PL011BTBase, 6, Pl011bt)
{
    AROS_LIBFUNC_INIT

    LONG result;

    if (owner == NULL)
        return PL011BT_ERR_ARGUMENT;

    ObtainSemaphore(&PL011BTBase->pl011bt_Sem);
    if (PL011BTBase->pl011bt_Owner != owner)
        result = PL011BT_ERR_NOT_OWNER;
    else
    {
        ULONG level = (enabled != 0) ^ PL011BTBase->pl011bt_PowerLow;

        if (PL011BTBase->pl011bt_PowerSoC)
        {
            /* Level before direction, so the pin does not glitch. */
            GPIOSet(PL011BTBase->pl011bt_PowerPin, level);
            GPIOSetFunc(PL011BTBase->pl011bt_PowerPin, GPIO_FSEL_OUTPUT);
            result = PL011BT_OK;
        }
        else
            result = firmware_gpio_set(PL011BTBase,
                                       PL011BTBase->pl011bt_PowerPin, level);
    }
    ReleaseSemaphore(&PL011BTBase->pl011bt_Sem);

    return result;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        long PL011BTWrite(

    SYNOPSIS
        void *owner, const void *data, unsigned long length)

    FUNCTION
        Write as many raw bytes as the UART transmit FIFO currently accepts.

    INPUTS
        owner - Non-NULL token holding the resource claim.
        data - Readable byte buffer, or NULL when length is zero.
        length - Maximum bytes to submit, no greater than LONG_MAX. The buffer
            must contain at least this many bytes and remain valid until return.

    RESULT
        Number of bytes accepted, from zero through length; or
        PL011BT_ERR_ARGUMENT for NULL owner or NULL data with nonzero length,
        or PL011BT_ERR_NOT_OWNER for a token without the claim.
        Zero is a valid short write, not an error.

    NOTES
        Requires a successfully configured UART and the appropriate controller
        power state; these are not checked. Stops when the FIFO is full and
        does not wait for room or transmission completion. Retry only the
        unaccepted suffix, with a caller-defined retry or timeout policy.

        No framing, packet boundaries or HCI status are added. Accepted bytes
        may still be queued in the UART when the function returns. The source
        buffer is not retained. Serialize writers and prevent configuration,
        power changes or release from racing with this call; it does not take
        the resource semaphore.

    SEE ALSO
        PL011BTConfigure(), PL011BTRead(), PL011BTRelease()

*****************************************************************************/
AROS_LH3(long, PL011BTWrite,
    AROS_LHA(void *, owner, A0),
    AROS_LHA(const void *, data, A1),
    AROS_LHA(unsigned long, length, D0),
    struct PL011BTBase *, PL011BTBase, 7, Pl011bt)
{
    AROS_LIBFUNC_INIT

    const UBYTE *bytes = data;
    ULONG written = 0;

    if (owner == NULL || (data == NULL && length != 0))
        return PL011BT_ERR_ARGUMENT;
    if (PL011BTBase->pl011bt_Owner != owner)
        return PL011BT_ERR_NOT_OWNER;

    /* Returns what the FIFO accepted, not a status: a caller with more to
     * send comes back rather than blocking the transport here. */
    while (written < length &&
           !(pl011bt_rd(PL011_0_BASE + PL011_FR) & PL011_FR_TXFF))
    {
        pl011bt_wr(PL011_0_BASE + PL011_DR, bytes[written]);
        written++;
    }
    return (LONG)written;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        long PL011BTRead(

    SYNOPSIS
        void *owner, void *data, unsigned long capacity)

    FUNCTION
        Remove available raw bytes from the interrupt-fed receive ring.

    INPUTS
        owner - Non-NULL token holding the resource claim.
        data - Writable byte buffer, or NULL when capacity is zero.
        capacity - Maximum bytes to receive, no greater than LONG_MAX. The
            buffer must provide at least this many bytes until return.

    RESULT
        Number of bytes copied, from zero through capacity; or
        PL011BT_ERR_ARGUMENT for NULL owner or NULL data with nonzero capacity,
        or PL011BT_ERR_NOT_OWNER for a token without the claim.
        Zero means no bytes were copied, not end of stream.

    NOTES
        Returns without waiting for new input. Requires configuration with a
        registered receive interrupt handler; verify PL011BT_CAP_RX_INTERRUPT
        after PL011BTConfigure(). The ring holds at most 8191 unread bytes.
        Drain it promptly to limit data loss. There is no client notification
        or wait operation in this interface.

        Transfers raw bytes without H:4 framing. Consumed bytes are removed
        from the ring. The destination is not retained. Use one reader and
        prevent configuration, power changes or release from racing with the
        call; it does not take the resource semaphore.

    BUGS
        UART error status and receive overflow are not returned to the caller.
        Bytes marked with UART errors can be delivered as ordinary data, and
        ring overflow can lose bytes. Diagnostic logging is not a reliable
        error-reporting interface. If interrupt registration fails, there is
        no direct FIFO read fallback.

    SEE ALSO
        PL011BTConfigure(), PL011BTGetCapabilities(), PL011BTWrite()

*****************************************************************************/
AROS_LH3(long, PL011BTRead,
    AROS_LHA(void *, owner, A0),
    AROS_LHA(void *, data, A1),
    AROS_LHA(unsigned long, capacity, D0),
    struct PL011BTBase *, PL011BTBase, 8, Pl011bt)
{
    AROS_LIBFUNC_INIT

    UBYTE *bytes = data;
    ULONG read = 0;

    if (owner == NULL || (data == NULL && capacity != 0))
        return PL011BT_ERR_ARGUMENT;
    if (PL011BTBase->pl011bt_Owner != owner)
        return PL011BT_ERR_NOT_OWNER;

    /*
     * From the ring, not from the FIFO.
     *
     * Reading the FIFO here would make the caller's scheduling the deadline
     * for the hardware, and no schedule is fast enough: OVERRUN was reported
     * on real hardware even at a one-millisecond poll. The interrupt owns the
     * FIFO and this owns the tail, so a slow reader costs latency, not data.
     */
    while (read < capacity &&
           PL011BTBase->pl011bt_RXTail != PL011BTBase->pl011bt_RXHead)
    {
        bytes[read] = PL011BTBase->pl011bt_RXRing[PL011BTBase->pl011bt_RXTail];
        PL011BTBase->pl011bt_RXTail =
            (PL011BTBase->pl011bt_RXTail + 1) & (PL011BT_RX_RING - 1);
        read++;
    }

    if (PL011BTBase->pl011bt_RXDropped != 0)
    {
        bug("[PL011BT] rx ring overflow, %lu byte(s) dropped\n",
            (ULONG)PL011BTBase->pl011bt_RXDropped);
        PL011BTBase->pl011bt_RXDropped = 0;
    }
    return (LONG)read;

    AROS_LIBFUNC_EXIT
}

/*
 * Register the init with genmodule's INITLIB set.
 *
 * Without this line pl011bt_init() is a static function nobody references, so
 * the compiler drops it along with everything only it calls -- and genmodule,
 * having no init to run, adds the resource anyway. OpenResource() then hands
 * out a base whose fields are all zero, which is worse than a missing
 * resource because it looks like a working one: PL011BTGetAPIVersion()
 * answers from a constant, and the first call that touches the base hangs.
 * ObtainSemaphore() on a zeroed SignalSemaphore blocks forever, because
 * InitSemaphore() sets ss_QueueCount to -1 and zero reads as "already owned".
 */
ADD2INITLIB(pl011bt_init, 0)
