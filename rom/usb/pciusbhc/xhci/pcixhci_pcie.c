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

#include LC_LIBDEFS_FILE

#define PCIXHCIBase unit->pcixhcibase

VOID PCIXHCI_PCIE(struct PCIXHCIUnit *unit) {

    IPTR PCIE_CapO, MSI_CapO, MSIX_CapO, PCIECap_SerialO;
    IPTR PCIE_Cap=0 , MSI_Cap=0 , MSIX_Cap=0 , PCIECap_SerialL=-1, PCIECap_SerialU=-1;

    OOP_GetAttr(unit->hc.pcidevice, aHidd_PCIDevice_CapabilityPCIE, (APTR)&PCIE_CapO);
    OOP_GetAttr(unit->hc.pcidevice, aHidd_PCIDevice_CapabilityMSIX, (APTR)&MSIX_CapO);
    OOP_GetAttr(unit->hc.pcidevice, aHidd_PCIDevice_CapabilityMSI,  (APTR)&MSI_CapO);
    OOP_GetAttr(unit->hc.pcidevice, aHidd_PCIDevice_ExtendedCapabilitySerialNumber, (APTR)&PCIECap_SerialO);

    if(PCIE_CapO)
        PCIE_Cap = HIDD_PCIDevice_ReadConfigWord(unit->hc.pcidevice, PCIE_CapO+2);

    if(MSIX_CapO)
        MSIX_Cap = HIDD_PCIDevice_ReadConfigWord(unit->hc.pcidevice, MSIX_CapO+2);

    if(MSI_CapO)
        MSI_Cap  = HIDD_PCIDevice_ReadConfigWord(unit->hc.pcidevice, MSI_CapO+2);

    if(PCIECap_SerialO) {
        PCIECap_SerialL = HIDD_PCIDevice_ReadConfigLong(unit->hc.pcidevice, PCIECap_SerialO+4);
        PCIECap_SerialU = HIDD_PCIDevice_ReadConfigLong(unit->hc.pcidevice, PCIECap_SerialO+8);
    }

    mybug_unit(-1, ("\n"));
    mybug_unit(-1, ("PCIECap_Serial %08x:%08x\n", PCIECap_SerialU, PCIECap_SerialL));
    mybug_unit(-1, ("PCIE_Cap %08x\n", PCIE_Cap));
    mybug_unit(-1, ("MSIX_Cap %08x\n", MSIX_Cap));
    mybug_unit(-1, ("MSI_Cap  %08x\n", MSI_Cap));

    mybug_unit(-1, ("\n"));




}




