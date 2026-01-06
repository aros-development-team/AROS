/*
 * ROM Resident early bootstrap routine(s) for Poseidon,
 * allowing the AROS ROM to use USB storage devices.
 * PsdStackloader should be started during startup-sequence nonetheless.
 */

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <exec/resident.h>

#include <proto/bootloader.h>
#include <proto/poseidon.h>
#include <proto/exec.h>

#include <string.h>

int __startup usbromstartup_entry(void)
{
    return -1;
}

static const char name[];
static const char version[];
static const UBYTE eendptr;

AROS_UFP3(static IPTR, usbromstartup_early,
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

const struct Resident usbEarlyResident __attribute__((used)) =
{
    RTC_MATCHWORD,
    (struct Resident *)&usbEarlyResident,
    (APTR)&eendptr,
    RTF_COLDSTART,
    41,
    NT_TASK,
    35,
    name,
    &version[5],
    (APTR)usbromstartup_early
};

static const char name[] = "Poseidon Early ROM Init";
static const char version[] = "$VER:Poseidon ROM Early-Startup v41.3";

#define USBROMSTART_FLAG_NOUSB  (1 << 0)
#define USBROMSTART_FLAG_NOUSB3 (1 << 1)
#define USBROMSTART_FLAG_NOBOOTMS (1 << 2)

AROS_UFH3(static IPTR, usbromstartup_early,
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    APTR BootLoaderBase;
    struct Library *ps;
    struct PsdHardware *phw;
    ULONG bootflags = 0;

    D(bug("[USBROMStartup] %s: Loading poseidon...\n", __func__));
    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase) {
        struct List *args = GetBootInfo(BL_Args);
        if (args) {
            struct Node *node;
            ForeachNode(args, node) {
                if (strncmp(node->ln_Name, "nousb3", 6) == 0) {
                    bootflags |= USBROMSTART_FLAG_NOUSB3;
                    continue;
                } 
                if (strncmp(node->ln_Name, "nousb", 5) == 0) {
                    bootflags |= USBROMSTART_FLAG_NOUSB;
                    continue;
                }
                if (strncmp(node->ln_Name, "nobootms", 8) == 0) {
                    bootflags |= USBROMSTART_FLAG_NOBOOTMS;
                }
            }
        }
    }

    if ((!(bootflags & USBROMSTART_FLAG_NOUSB)) &&
          (ps = OpenLibrary("poseidon.library", 4))) {
        APTR msdclass = NULL;
        IPTR usecount = 0;
        ULONG bootdelay = 4;
        ULONG cnt = 0;

        D(bug("[USBROMStartup] %s: Adding early ROM classes...\n", __func__));

        psdAddClass("hub.class", 0);
        if (!(bootflags & USBROMSTART_FLAG_NOBOOTMS))
            msdclass = psdAddClass("massstorage.class", 0);

        /* Find available usb hardware */
        while((phw = psdAddHardware("pciusb.device", cnt))) {
            D(bug("[USBROMStartup] %s: Added pciusb.device unit %u\n", __func__, cnt));
            psdEnumerateHardware(phw);
            cnt++;
        }

        if (!(bootflags & USBROMSTART_FLAG_NOUSB3)) {
            psdAddClass("hubss.class", 0);
            cnt = 0;
            while((phw = psdAddHardware("pcixhci.device", cnt))) {
                D(bug("[USBROMStartup] %s: Added pcixhci.device unit %u\n", __func__, cnt));
                psdEnumerateHardware(phw);
                cnt++;
            }
        }

        D(bug("[USBROMStartup] %s: Binding classes...\n", __func__));
        psdClassScan();

        if(msdclass) {
            psdDelayMS(1000); // wait for hubs to settle
            psdGetAttrs(PGA_USBCLASS, msdclass, UCA_UseCount, &usecount, TAG_END);
            if(usecount > 0) {
                psdAddErrorMsg(RETURN_OK, (STRPTR)name,
                               "Delaying further execution by %ld second(s) (boot delay).",
                               bootdelay);
                if(bootdelay > 1) {
                    psdDelayMS((bootdelay-1)*1000);
                }
            } else {
                psdAddErrorMsg(RETURN_OK, (STRPTR)name,
                               "Boot delay skipped, no mass storage devices found.");
            }
        }
        CloseLibrary(ps);
    }
    return 0;

    AROS_USERFUNC_EXIT
}

static const UBYTE eendptr = 0;
