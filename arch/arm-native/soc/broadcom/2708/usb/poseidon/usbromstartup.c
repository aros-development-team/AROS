/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

/* Very basic bootstrap for Poseidon in AROS kernel for enabling of USB booting and HID devices.
 * PsdStackloader should be started during startup-sequence nonetheless */

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
    /* Run after intuition (residentpri 15) so poseidon.library is initialised. */
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

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        APTR msdclass;
        IPTR usecount = 0;
        ULONG bootdelay = 4;

        D(bug("[USBROMStartup] Adding classes...\n"));

        psdAddClass("hub.class", 0);
        msdclass = psdAddClass("massstorage.class", 0);
        /* hid/bootmouse/bootkeyboard are loaded from disk by AddUSBClasses in Startup-Sequence */

        D(bug("[USBROMStartup] Added chipset drivers...\n"));

        /* load the raspi usb hardware driver */
        if ((phw = psdAddHardware("usb2otg.device", 0)))
        {
            D(bug("[USBROMStartup] Added usb2otg.device unit %u\n", 0));

            psdEnumerateHardware(phw);
        }

        D(bug("[USBROMStartup] Scanning classes...\n"));
        psdClassScan();
        D(bug("[USBROMStartup] classes enumerated\n"));

        if(msdclass)
        {
            D(bug("[USBROMStartup] waiting for hubs..\n"));
            psdDelayMS(1000); // wait for hubs to settle
            D(bug("[USBROMStartup] checking for massstorage devices..\n"));
            psdGetAttrs(PGA_USBCLASS, msdclass, UCA_UseCount, &usecount, TAG_END);
            D(bug("[USBROMStartup] %d masstorage devices found\n", usecount));
            if(usecount > 0)
            {
                D(bug("[USBROMStartup] adding boot delay\n"));

                psdAddErrorMsg(RETURN_OK, (STRPTR)name,
                               "Delaying further execution by %ld second(s) (boot delay).",
                               bootdelay);
                if(bootdelay > 1)
                {
                    psdDelayMS((bootdelay-1)*1000);
                }
            } else {
                psdAddErrorMsg(RETURN_OK, (STRPTR)name, "Boot delay skipped, no mass storage devices found.");
            }
        }

        D(bug("[USBROMStartup] cleaning up .. \n"));
        CloseLibrary(ps);
    }
    D(bug("[USBROMStartup] Finished...\n"));

    return 0;

    AROS_USERFUNC_EXIT
}

static const UBYTE endptr = 0;
