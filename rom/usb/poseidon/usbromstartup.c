/* Very basic bootstrap for Poseidon in AROS kernel for enabling of USB booting and HID devices.
 * PsdStackloader should be started during startup-sequence nonetheless */

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

#include <proto/poseidon.h>
#include <proto/exec.h>

#include <string.h>

int usbromstartup_init(void)
{
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

	D(bug("[USBROMStartup] Scanning classes...\n"));
        psdClassScan();

        if(msdclass)
        {
            psdDelayMS(1000); // wait for hubs to settle
            psdGetAttrs(PGA_USBCLASS, msdclass, UCA_UseCount, &usecount, TAG_END);
            if(usecount > 0)
            {
                psdAddErrorMsg(RETURN_OK, MOD_NAME_STRING,
                               "Delaying further execution by %ld second(s) (boot delay).",
                               bootdelay);
                if(bootdelay > 1)
                {
                    psdDelayMS((bootdelay-1)*1000);
                }
            } else {
                psdAddErrorMsg(RETURN_OK, MOD_NAME_STRING, "Boot delay skipped, no mass storage devices found.");
            }
        }

        CloseLibrary(ps);
    }
    return(0);
}

ADD2INITLIB(usbromstartup_init, 0)
