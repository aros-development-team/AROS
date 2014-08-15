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

#include LC_LIBDEFS_FILE

BOOL PCIXHCI_HCReset(struct PCIXHCIUnit *unit) {
    mybug_unit(-1, ("Resetting the host controller\n"));

    return TRUE;
}

BOOL PCIXHCI_HCInit(struct PCIXHCIUnit *unit) {
    mybug(-1, ("[PCIXHCI] PCIXHCI_Examine: pcidevice = %p\n", unit->hc.pcidevice));
    mybug(-1, ("[PCIXHCI] PCIXHCI_Examine: pcidriver = %p\n", unit->hc.pcidriver));
    mybug(-1, ("[PCIXHCI] PCIXHCI_Examine: bus       = %x\n", unit->hc.bus));
    mybug(-1, ("[PCIXHCI] PCIXHCI_Examine: dev       = %x\n", unit->hc.dev));
    mybug(-1, ("[PCIXHCI] PCIXHCI_Examine: sub       = %x\n", unit->hc.sub));
    mybug(-1, ("[PCIXHCI] PCIXHCI_Examine: intline   = %d\n", unit->hc.intline));
    mybug(-1, ("[PCIXHCI] PCIXHCI_Examine: pcibase0  = %p\n", unit->hc.pcibase0));

    return TRUE;
}
