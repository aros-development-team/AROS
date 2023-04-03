/* pciusb_init.c - generic pciusb init code for AROS
*/

#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <exec/types.h>

#include <proto/bootloader.h>
#include <proto/exec.h>

#include <string.h>

#include "pciusb.h"

/*
 * Process some AROS-specific arguments.
 * 'usbpoweron' helps to bring up USB ports on IntelMac,
 * whose firmware sets them up incorrectly.
 */
static int getArguments(struct PCIDevice *base)
{
    APTR BootLoaderBase;

    BootLoaderBase = OpenResource("bootloader.resource");

    KPRINTF(20, ("bootloader @ 0x%p\n", BootLoaderBase));

    if (BootLoaderBase)
    {
        struct List *args = GetBootInfo(BL_Args);

        if (args)
        {
            struct Node *node;

            ForeachNode(args, node)
            {
                if (strncmp(node->ln_Name, "USB=", 4) == 0)
                {
                    const char *CmdLine = &node->ln_Name[3];

                    if (strstr(CmdLine, "forcepower"))
                    {
                        base->hd_Flags |= HDF_FORCEPOWER;
                        continue;
                    }
#if defined(TMPXHCICODE)
                    if (strstr(CmdLine, "xhci"))
                    {
                        base->hd_Flags |= HDF_ENABLEXHCI;
                    }
#endif
                }
            }
        }
    }
    if (base->hd_Flags & HDF_FORCEPOWER)
    {
        D(bug("[PCIUSB] %s: Forcing USB Power\n", __func__));
    }
#if defined(TMPXHCICODE)
    if (base->hd_Flags & HDF_ENABLEXHCI)
    {
        D(bug("[PCIUSB] %s: Enabling experimental XHCI code\n", __func__));
    }
#endif
    return TRUE;
}

ADD2INITLIB(getArguments, 10)
