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
    if (BootLoaderBase)
    {
        struct List *args = GetBootInfo(BL_Args);

        if (args)
        {
            struct Node *node;

            for (node = args->lh_Head; node->ln_Succ; node = node->ln_Succ)
            {
                if (stricmp(node->ln_Name, "forceusbpower") == 0)
                {
                    base->hd_Flags = HDF_FORCEPOWER;
                    break;
                }
            }
        }
    }

    return TRUE;
}

ADD2INITLIB(getArguments, 10)
