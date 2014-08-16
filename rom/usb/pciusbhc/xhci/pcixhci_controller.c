/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI XHCI USB host controller
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <aros/io.h>
#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/stdc.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>

#include <asm/io.h>
#include <inttypes.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include "pcixhci_intern.h"

#include "pcixhci_controller.h"

#include LC_LIBDEFS_FILE

#define PCIXHCIBase unit->pcixhcibase

/*
    We get called only once when the driver inits
    We own the controller until our driver expunges so we assume that nobody messes with our stuff...
*/
BOOL PCIXHCI_HCInit(struct PCIXHCIUnit *unit) {
    mybug(0, ("[PCIXHCI] PCIXHCI_HCInit: Entering function\n"));

    /* Our unit is in suspended state until it is reset */
    unit->state = UHSF_SUSPENDED;

    snprintf(unit->name, 255, "PCIXHCI[%02x:%02x.%01x]", (UBYTE)unit->hc.bus, (UBYTE)unit->hc.dev, (UBYTE)unit->hc.sub);
    unit->node.ln_Name = (STRPTR)&unit->name;

    /* Store opregbase */
    unit->hc.opregbase = (APTR) ((ULONG) (unit->hc.capregbase) + capreg_readb(XHCI_CAPLENGTH));

    mybug(-1, ("[PCIXHCI] PCIXHCI_HCInit: unit node name %s\n", unit->node.ln_Name));
    mybug(-1, ("[PCIXHCI] PCIXHCI_HCInit: pcidevice = %p\n",    unit->hc.pcidevice));
    mybug(-1, ("[PCIXHCI] PCIXHCI_HCInit: pcidriver = %p\n",    unit->hc.pcidriver));
    mybug(-1, ("[PCIXHCI] PCIXHCI_HCInit: bus       = %x\n",    unit->hc.bus));
    mybug(-1, ("[PCIXHCI] PCIXHCI_HCInit: dev       = %x\n",    unit->hc.dev));
    mybug(-1, ("[PCIXHCI] PCIXHCI_HCInit: sub       = %x\n",    unit->hc.sub));
    mybug(-1, ("[PCIXHCI] PCIXHCI_HCInit: intline   = %d\n",    unit->hc.intline));
    mybug(-1, ("[PCIXHCI] PCIXHCI_HCInit: capregbase  = %p\n",  unit->hc.capregbase));
    mybug(-1, ("[PCIXHCI] PCIXHCI_HCInit: opregbase  = %p\n",   unit->hc.opregbase));

    struct TagItem pciActivateMemAndBusmaster[] = {
            { aHidd_PCIDevice_isIO,     FALSE },
            { aHidd_PCIDevice_isMEM,    TRUE },
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    OOP_SetAttrs(unit->hc.pcidevice, (struct TagItem *)pciActivateMemAndBusmaster);

    NEWLIST(&unit->roothub.port_list);

    return TRUE;
}

BOOL PCIXHCI_HCReset(struct PCIXHCIUnit *unit) {
    mybug(0, ("[PCIXHCI] PCIXHCI_HCReset: Entering function\n"));

    /* our unit is in reset state until the higher level usb reset is called */
    unit->state = UHSF_RESET;







    return TRUE;
}









