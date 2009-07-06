/* Very basic bootstrap for Poseidon in AROS kernel for enabling of USB booting and HID devices. 
 * PsdStackloader should be started during startup-sequence nonetheless */

#include <aros/symbolsets.h>

#include <proto/poseidon.h>
#include <proto/exec.h>

int usbromstartup_init(void)
{
    struct Library *ps;
    struct PsdHardware *phw;
    ULONG cnt = 0;

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdAddClass("hub.class", 0);
        psdAddClass("hid.class", 0);
        psdAddClass("massstorage.class", 0);

        /* now this finds all usb hardware pci cards */
        while((phw = psdAddHardware("pciusb.device", cnt++)))
        {
            psdEnumerateHardware(phw);
        }

        psdClassScan();

        CloseLibrary(ps);
    }
    return(0);
}

ADD2INITLIB(usbromstartup_init, 0)
