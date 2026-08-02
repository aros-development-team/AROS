/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Brings Poseidon up from ROM, before any disk is mounted.
*/

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

/* OpenLibrary() only searches LibList, so a RTF_AUTOINIT resident the
   COLDSTART pass has not reached yet is never found */
static void prewarm_library(STRPTR libname, struct ExecBase *SysBase)
{
    struct Resident *res;

    if (FindName(&SysBase->LibList, libname))
        return;

    if ((res = FindResident(libname)) != NULL)
    {
        D(bug("[USBROMStartup] prewarm InitResident(\"%s\")\n", libname));
        InitResident(res, BNULL);
    }
    else
        bug("[USBROMStartup] prewarm: %s not in resident list\n", libname);
}

const struct Resident usbHook =
{
    RTC_MATCHWORD,
    (struct Resident *)&usbHook,
    (APTR)&endptr,
    RTF_COLDSTART,
    41,
    NT_TASK,
    /* After intuition (15), so poseidon.library has been initialised */
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

    if ((ps = OpenLibrary("poseidon.library", 4)) != NULL)
    {
        APTR msdclass;
        IPTR usecount = 0;
        ULONG bootdelay = 4;

        D(bug("[USBROMStartup] Adding classes...\n"));

        psdAddClass("hub.class", 0);
        msdclass = psdAddClass("massstorage.class", 0);
        psdAddClass("hid.class", 0);
        psdAddClass("bootkeyboard.class", 0);
        psdAddClass("bootmouse.class", 0);

        D(bug("[USBROMStartup] Adding chipset driver...\n"));

        if ((phw = psdAddHardware("pcixhci.device", 0)) != NULL)
        {
            D(bug("[USBROMStartup] Added pcixhci.device unit %u\n", 0));

            psdEnumerateHardware(phw);
        }
        else
            D(bug("[USBROMStartup] no pcixhci.device\n"));

        D(bug("[USBROMStartup] Scanning classes...\n"));
        psdClassScan();
        D(bug("[USBROMStartup] classes enumerated\n"));

        if (msdclass)
        {
            /*
             * Only a moment for the hubs to settle. There is no point
             * waiting on enumeration here: massstorage adds a boot node
             * of its own whenever a volume turns up, and dosboot keeps
             * retrying until something bootable does, so a device that
             * arrives late is found anyway.
             */
            psdDelayMS(1000);
            psdGetAttrs(PGA_USBCLASS, msdclass, UCA_UseCount, &usecount,
                        TAG_END);
            D(bug("[USBROMStartup] %d massstorage device(s) so far\n",
                  usecount));

            if (usecount > 0)
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR)name,
                               "Delaying further execution by %ld second(s) (boot delay).",
                               bootdelay);
                if (bootdelay > 1)
                    psdDelayMS((bootdelay - 1) * 1000);
            }
            else
                psdAddErrorMsg(RETURN_OK, (STRPTR)name,
                               "Boot delay skipped, no mass storage devices found.");
        }

        D(bug("[USBROMStartup] cleaning up ..\n"));
        CloseLibrary(ps);
    }
    D(bug("[USBROMStartup] Finished...\n"));

    return 0;

    AROS_USERFUNC_EXIT
}

static const UBYTE endptr = 0;
