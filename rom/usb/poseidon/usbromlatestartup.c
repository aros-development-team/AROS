/*
 * ROM Resident late bootstrap startup routine(s) for Poseidon,
 * allowing the AROS ROM to use HID devices.
 */

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/resident.h>
#include <proto/poseidon.h>
#include <proto/exec.h>

static const char name[];
static const char version[];
static const UBYTE lendptr;

static const char version[] = "$VER:Poseidon ROM Late-Startup v41.2";

AROS_UFP3(static IPTR, usbromstartup_late,
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

const struct Resident usbLateResident __attribute__((used)) =
{
    RTC_MATCHWORD,
    (struct Resident *)&usbLateResident,
    (APTR)&lendptr,
    RTF_COLDSTART,
    41,
    NT_TASK,
    28,
    name,
    &version[5],
    (APTR)usbromstartup_late
};

static const char name[] = "Poseidon Late ROM Init";

AROS_UFH3(static IPTR, usbromstartup_late,
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Library *ps;

    D(bug("[USBROMStartup] %s()\n", __func__));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
	D(bug("[USBROMStartup] %s: Adding late classes...\n", __func__));

        if(!(psdAddClass("hid.class", 0)))
        {
            psdAddClass("bootmouse.class", 0);
            psdAddClass("bootkeyboard.class", 0);
        }

	D(bug("[USBROMStartup] %s: Binding classes...\n", __func__));
        psdClassScan();

        CloseLibrary(ps);
    }
    return 0;

    AROS_USERFUNC_EXIT
}

static const UBYTE lendptr = 0;
