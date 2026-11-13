/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pwrbutton.resource -- the Raspberry Pi 5/500 power button.

    On the Pi 400 the power key is a USB HID usage, handled by hid.class.
    On BCM2712 boards the button (the Pi 500's power key included) is wired
    to SoC GPIO 20 instead, which the device tree describes as /pwr_button.
    A press asks for confirmation and powers the machine off.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/symbolsets.h>
#include <devices/timer.h>
#include <exec/pm.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/openfirmware.h>

#include LC_LIBDEFS_FILE

#define BCM2712_GIO_BASE        0x107D508500ULL
#define BCM2712_GIO_DATA        0x04
#define BCM2712_GIO_IODIR       0x08    /* 1 = input */
#define BCM2712_PINCTRL_BASE    0x107D504100ULL

#define PWR_PIN                 20
#define PWR_PULL_UP             2
#define PWR_POLL_US             100000

/* GPIO 20 mux and pull fields per stepping, from OpenBSD bcmstbpinctrl.c (ISC) */
struct pwr_pinctrl
{
    UBYTE muxreg, muxbit, pullreg, pullbit;
};

static const struct pwr_pinctrl pwr_c0 = { 0x08, 16, 0x20, 24 };
static const struct pwr_pinctrl pwr_d0 = { 0x04, 16, 0x14,  4 };

static BOOL pwr_activelow;

static void pwr_rmw(ULONG reg, ULONG mask, ULONG val)
{
    volatile ULONG *r = (volatile ULONG *)(BCM2712_PINCTRL_BASE + reg);

    *r = AROS_LONG2LE((AROS_LE2LONG(*r) & ~mask) | val);
}

static BOOL pwr_pressed(void)
{
    ULONG data = AROS_LE2LONG(*(volatile ULONG *)(BCM2712_GIO_BASE + BCM2712_GIO_DATA));

    return ((data >> PWR_PIN) & 1) != pwr_activelow;
}

static void pwr_confirm(void)
{
    struct Library *IntuitionBase;
    struct EasyStruct es;
    LONG res = 0;

    if ((IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        es.es_StructSize = sizeof(struct EasyStruct);
        es.es_Flags = 0;
        es.es_Title = "Power Button";
        es.es_TextFormat = "Shut down the system?";
        es.es_GadgetFormat = "Shut Down|Cancel";

        res = EasyRequestArgs(NULL, &es, NULL, NULL);
        CloseLibrary(IntuitionBase);
    }

    if (res)
        ShutdownA(SD_ACTION_POWEROFF);
}

static void pwr_task(void)
{
    struct timerequest *tr = NULL;
    struct MsgPort *port;
    BOOL was = TRUE;    /* a press still held from boot is not a request */

    if (!(port = CreateMsgPort()))
        return;
    if (!(tr = (struct timerequest *)CreateIORequest(port, sizeof(*tr)))
        || OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0))
    {
        bug("[PwrButton] no timer\n");
        DeleteIORequest((struct IORequest *)tr);
        DeleteMsgPort(port);
        return;
    }

    for (;;)
    {
        BOOL now;

        tr->tr_node.io_Command = TR_ADDREQUEST;
        tr->tr_time.tv_secs    = 0;
        tr->tr_time.tv_micro   = PWR_POLL_US;
        DoIO((struct IORequest *)tr);

        now = pwr_pressed();
        if (now && !was)
        {
            D(bug("[PwrButton] pressed\n"));
            pwr_confirm();
        }
        was = now;
    }
}

static int pwr_init(LIBBASETYPEPTR PwrButtonBase)
{
    APTR OpenFirmwareBase = OpenResource("openfirmware.resource");
    const struct pwr_pinctrl *pc;
    void *key, *prop;
    const ULONG *cell;

    if (!OpenFirmwareBase ||
        !(key = OF_OpenKey("/pwr_button/pwr")) ||
        !(prop = OF_FindProperty(key, "gpios")) ||
        OF_GetPropLen(prop) < 3 * sizeof(ULONG))
        return FALSE;

    if (OF_FindNodeByCompatible(NULL, "brcm,bcm2712d0-pinctrl"))
        pc = &pwr_d0;
    else if (OF_FindNodeByCompatible(NULL, "brcm,bcm2712c0-pinctrl") ||
             OF_FindNodeByCompatible(NULL, "brcm,bcm2712-pinctrl"))
        pc = &pwr_c0;
    else
        return FALSE;

    /* Only the SoC GPIO 20 wiring of every BCM2712 board so far is known */
    cell = (const ULONG *)OF_GetPropValue(prop);
    if (AROS_BE2LONG(cell[1]) != PWR_PIN)
        return FALSE;
    pwr_activelow = (AROS_BE2LONG(cell[2]) & 1) != 0;

    /* GPIO function, pull-up, input: what the tree's pwr_button_pins ask for */
    pwr_rmw(pc->muxreg, 0xfu << pc->muxbit, 0);
    pwr_rmw(pc->pullreg, 0x3u << pc->pullbit, PWR_PULL_UP << pc->pullbit);
    *(volatile ULONG *)(BCM2712_GIO_BASE + BCM2712_GIO_IODIR) |= AROS_LONG2LE(1u << PWR_PIN);

    D(bug("[PwrButton] GPIO %d, active %s\n", PWR_PIN, pwr_activelow ? "low" : "high"));

    return NewCreateTask(TASKTAG_PC, pwr_task,
                         TASKTAG_NAME, "Power Button",
                         TASKTAG_PRI, 0,
                         TAG_DONE) != NULL;
}

ADD2INITLIB(pwr_init, 0)
