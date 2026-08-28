/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi platform xHCI discovery override for pcixhci.device
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "pcixhci.h"

void pcixhci_probe_platform(struct PCIDevice *hd)
{
    struct Library *rp1usb = OpenResource("rp1usb.resource");
    if (rp1usb) {
        IPTR *fields = (IPTR *)((UBYTE *)rp1usb + sizeof(struct Library));
        if (fields[2]) { /* present */
            IPTR bases[2] = { fields[0], fields[1] }; /* usb0, usb1 */
            int i;
            for (i = 0; i < 2; i++) {
                if (bases[i]) {
                    struct PCIController *hc = AllocPooled(hd->hd_MemPool, sizeof(struct PCIController));
                    if (hc) {
                        hc->hc_Device = hd;
                        hc->hc_RegBase = (volatile APTR)bases[i];
                        hc->hc_Flags = HCF_PLATFORM;
                        hc->hc_FunctionNum = i;
                        hc->hc_PCIIntLine = 0;
                        hc->hc_DevID = 0x1DE40001 + i;
                        AddTail(&hd->hd_TempHCIList, &hc->hc_Node);
                        pciusbDebug("PCI", "RP1 platform xHCI%d @ 0x%p\n", i, (APTR)bases[i]);
                    }
                }
            }
        }
    }
}
