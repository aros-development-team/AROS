/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi platform xHCI discovery override for pcixhci.device
*/

/* Bring-up diagnostics: pciusbDebug() needs DEBUG > 0 in this unit. */
#define DEBUG 1

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/rp1usb.h>
#include <aros/symbolsets.h>

#include "pcixhci.h"

void pcixhci_probe_platform(struct PCIDevice *hd)
{
    struct Library *RP1USBBase = OpenResource("rp1usb.resource");

    if (RP1USBBase)
    {
        IPTR dmaoff = RP1USBGetDMAOffset();
        int i;

        for (i = 0; i < 2; i++)
        {
            IPTR base = RP1USBGetBase(i);

            if (!base)
                continue;
            
            struct PCIController *hc = AllocPooled(hd->hd_MemPool, sizeof(struct PCIController));
            if (hc) {
                hc->hc_Device = hd;
                hc->hc_RegBase = (volatile APTR)base;
                hc->hc_Flags = HCF_PLATFORM;
                hc->hc_FunctionNum = i;
                /* A GIC INTID, not a PCI interrupt line.  0 if MSI is down. */
                hc->hc_PCIIntLine = RP1USBGetIRQ(i);
                hc->hc_DevID = 0x1DE40001 + i;
                hc->hc_PlatformDMAOffset = dmaoff;
                hc->hc_IsoPTDCount = PCIUSB_ISO_PTD_COUNT;

                NewList(&hc->hc_PeriodicTDQueue);
                NewList(&hc->hc_CtrlXFerQueue);
                NewList(&hc->hc_IntXFerQueue);
                NewList(&hc->hc_IsoXFerQueue);
                NewList(&hc->hc_BulkXFerQueue);
                NewList(&hc->hc_TDQueue);
                NewList(&hc->hc_AbortQueue);
                NewMinList(&hc->hc_RTIsoHandlers);

                AddTail(&hd->hd_TempHCIList, &hc->hc_Node);
                pciusbDebug("PCI", "RP1 platform xHCI%d @ 0x%p\n", i, (APTR)base);
            }
        }
    }
}
