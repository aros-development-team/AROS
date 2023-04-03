/* pciusb_init.c - pciusb arch specific init code for AROS-pc
*/

#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <exec/types.h>

#include <proto/bootloader.h>
#include <proto/exec.h>

#include <string.h>

#define __INLINE_ACPICA_STACKCALL__
#include <proto/acpica.h>

#include "pciusb.h"

/*
 * Process some AROS-specific arguments.
 * 'usbpoweron' helps to bring up USB ports on IntelMac,
 * whose firmware sets them up incorrectly.
 */
static int getArguments(struct PCIDevice *base)
{
    APTR BootLoaderBase;
    struct Library *ACPICABase;
    if ((ACPICABase = OpenLibrary("acpica.library", 0))) {
        /*
         * Use ACPI IDs to identify known machines which need HDF_FORCEPOWER to work.
         * Currently we know only MacMini.
         */
        ACPI_TABLE_HEADER *dsdt;
        ACPI_STATUS err;

        err = AcpiGetTable("DSDT", 1, &dsdt);
        if (err == AE_OK) {
            /* Yes, the last byte in ID is zero */
            if (strcmp(dsdt->OemTableId, "Macmini") == 0)
            {
                base->hd_Flags |= HDF_FORCEPOWER;
            }
        }
        CloseLibrary(ACPICABase);
        ACPICABase = NULL;
    }

    BootLoaderBase = OpenResource("bootloader.resource");
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
