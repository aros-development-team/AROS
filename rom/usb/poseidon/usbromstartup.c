/* Very basic bootstrap for Poseidon in AROS kernel for enabling of USB booting and HID devices.
 * PsdStackloader should be started during startup-sequence nonetheless */

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

const struct Resident usbHook =
{
    RTC_MATCHWORD,
    (struct Resident *)&usbHook,
    (APTR)&endptr,
    RTF_COLDSTART,
    41,
    NT_TASK,
    35,
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
    ULONG cnt = 0;

    D(bug("[USBROMStartup] Loading poseidon...\n"));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        APTR msdclass;
        IPTR usecount = 0;
        ULONG bootdelay = 4;

	D(bug("[USBROMStartup] Adding classes...\n"));

        psdAddClass("hub.class", 0);
        if(!(psdAddClass("hid.class", 0)))
        {
            psdAddClass("bootmouse.class", 0);
            psdAddClass("bootkeyboard.class", 0);
        }
        msdclass = psdAddClass("massstorage.class", 0);

        /* now this finds all usb hardware pci cards */
        while((phw = psdAddHardware("pciusb.device", cnt)))
        {
            D(bug("[USBROMStartup] Added pciusb.device unit %u\n", cnt));

            psdEnumerateHardware(phw);
            cnt++;
        }

        if (cnt == 0) {
            /* now this finds all other usb hardware pci cards */
            while((phw = psdAddHardware("ehci.device", cnt)))
            {
                D(bug("[USBROMStartup] Added ehci.device unit %u\n", cnt));

                psdEnumerateHardware(phw);
                cnt++;
            }
            cnt = 0;
            while((phw = psdAddHardware("ohci.device", cnt)))
            {
                D(bug("[USBROMStartup] Added ohci.device unit %u\n", cnt));

                psdEnumerateHardware(phw);
                cnt++;
            }
            cnt = 0;
            while((phw = psdAddHardware("uhci.device", cnt)))
            {
                D(bug("[USBROMStartup] Added uhci.device unit %u\n", cnt));

                psdEnumerateHardware(phw);
                cnt++;
            }
        }

	D(bug("[USBROMStartup] Scanning classes...\n"));
        psdClassScan();

        if(msdclass)
        {
            psdDelayMS(1000); // wait for hubs to settle
            psdGetAttrs(PGA_USBCLASS, msdclass, UCA_UseCount, &usecount, TAG_END);
            if(usecount > 0)
            {
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

        CloseLibrary(ps);
    }
    return 0;

    AROS_USERFUNC_EXIT
}

static const UBYTE endptr = 0;
