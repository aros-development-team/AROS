/* Very basic bootstrap for Poseidon in AROS kernel for enabling of USB booting and HID devices.
 * PsdStackloader should be started during startup-sequence nonetheless */

#include <aros/symbolsets.h>
#include <aros/bootloader.h>

#include LC_LIBDEFS_FILE

#include <proto/poseidon.h>
#include <proto/exec.h>
#include <proto/bootloader.h>

#include <string.h>

int usbromstartup_init(void)
{
    struct Library *ps;
    struct PsdHardware *phw;
    ULONG cnt = 0;
    APTR BootLoaderBase;

    // for now, only enable USB during boot if enableusb kernel parameter has been given
    if((BootLoaderBase = OpenResource("bootloader.resource")))
    {
        struct List *args = GetBootInfo(BL_Args);
        BOOL enable = FALSE;
        if(args)
        {
            struct Node *node;
            for(node = args->lh_Head; node->ln_Succ; node = node->ln_Succ)
            {
                if(stricmp(node->ln_Name, "enableusb") == 0)
                {
                    enable = TRUE;
                }
            }
        }
        if(!enable)
        {
            return(0);
        }
    }
    
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        APTR msdclass;
        IPTR usecount = 0;
        ULONG bootdelay = 4;

        psdAddClass("hub.class", 0);
        if(!(psdAddClass("hid.class", 0)))
        {
            psdAddClass("bootmouse.class", 0);
            psdAddClass("bootkeyboard.class", 0);
        }
        msdclass = psdAddClass("massstorage.class", 0);

        /* now this finds all usb hardware pci cards */
        while((phw = psdAddHardware("pciusb.device", cnt++)))
        {
            psdEnumerateHardware(phw);
        }

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
