/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

/* Minimal Pi-only Poseidon bootstrap: registers the USB controller
 * (usb2otg.device) with poseidon.library at coldsart. USB classes are
 * disk-loaded by AddUSBClasses from Startup-Sequence, which
 * triggers class scan / binding. */

#define DEBUG 0

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/resident.h>
#include <proto/poseidon.h>
#include <proto/exec.h>

int __startup usbromstartup_entry(void)
{
    return -1;
}

static const char name[];
static const char version[];
static const UBYTE endptr;

AROS_UFP3(static IPTR, usbromstartup_init,
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

/* Force a resident-list library to be initialised (its lib_init runs and it
 * gets added to SysBase->LibList). Needed because AROS's OpenLibrary only
 * looks in LibList - it does not trigger InitResident on RTF_AUTOINIT
 * residents that haven't been COLDSTART-iterated yet. */
static void prewarm_library(STRPTR name, struct ExecBase *SysBase)
{
    if (FindName(&SysBase->LibList, name))
        return;
    {
        struct Resident *res = FindResident(name);
        if (res)
        {
            D(bug("[USBROMStartup] prewarm InitResident(\"%s\")\n", name));
            InitResident(res, BNULL);
        }
        else
        {
            bug("[USBROMStartup] prewarm: %s not in resident list\n", name);
        }
    }
}

const struct Resident usbHook =
{
    RTC_MATCHWORD,
    (struct Resident *)&usbHook,
    (APTR)&endptr,
    RTF_COLDSTART,
    41,
    NT_TASK,
    /* Run after intuition (residentpri 15). */
    10,
    name,
    &version[5],
    (APTR)usbromstartup_init
};

static const char name[] = "Poseidon ROM starter";
static const char version[] = "$VER:Poseidon ROM startup v41.1";

AROS_UFH3(static IPTR, usbromstartup_init,
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Library *ps;
    struct PsdHardware *phw;

    D(bug("[USBROMStartup] Loading poseidon...\n"));

    prewarm_library("poseidon.library", SysBase);

    D(bug("[USBROMStartup] opening poseidon.library...\n"));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        if ((phw = psdAddHardware("usb2otg.device", 0)))
        {
            D(bug("[USBROMStartup] Added usb2otg.device unit %u\n", 0));
            psdEnumerateHardware(phw);
        }
        else
        {
            bug("[USBROMStartup] psdAddHardware failed\n");
        }

        CloseLibrary(ps);
    }
    else
    {
        D(bug("[USBROMStartup] OpenLibrary(poseidon.library) failed\n"));
    }
    D(bug("[USBROMStartup] Finished...\n"));

    return 0;

    AROS_USERFUNC_EXIT
}

static const UBYTE endptr = 0;
