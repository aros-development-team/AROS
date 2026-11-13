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
 * 'USB=forcepower' helps to bring up USB ports on IntelMac,
 * whose firmware sets them up incorrectly.
 */
static int getArguments(struct PCIDevice *base)
{
    APTR BootLoaderBase;

#if defined(AROS_USE_LOGRES)
#ifdef LogResBase
#undef LogResBase
#endif
#ifdef LogResHandle
#undef LogResHandle
#endif
    APTR LogResBase;
#define LogHandle (base->hd_LogRHandle)
    base->hd_LogResBase = OpenResource("log.resource");
    if (base->hd_LogResBase) {
        LogResBase = base->hd_LogResBase;
        base->hd_LogRHandle = logInitialise(&base->hd_Device.dd_Library.lib_Node);
    }
#endif
    BootLoaderBase = OpenResource("bootloader.resource");

    pciusbDebug("", "bootloader @ 0x%p\n", BootLoaderBase);

    if (BootLoaderBase) {
        struct List *args = GetBootInfo(BL_Args);

        if (args) {
            struct Node *node;

            ForeachNode(args, node) {
                if (strncmp(node->ln_Name, "USB=", 4) == 0) {
                    const char *CmdLine = &node->ln_Name[3];

                    if (strstr(CmdLine, "forcepower")) {
                        base->hd_Flags |= HDF_FORCEPOWER;
                        continue;
                    }
                }
            }
        }
    }
    if (base->hd_Flags & HDF_FORCEPOWER) {
        pciusbInfo("", "Forcing USB Power\n");
    }
    return TRUE;
}

ADD2INITLIB(getArguments, 10)
