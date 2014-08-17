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

/*
    Keep this one short and simple...
*/
static AROS_UFH3(void, GM_UNIQUENAME(Enumerator), AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(OOP_Object *, pciDevice, A2), AROS_UFHA(APTR, message, A1)) {
    AROS_USERFUNC_INIT

    LIBBASETYPE *LIBBASE = (LIBBASETYPE *)hook->h_Data;

    mybug(-1, ("\n[PCIXHCI] Enumerator: Found PCI XHCI host controller\n"));

    struct PCIXHCIUnit *unit;
    static ULONG unitnum = 0;

    unit = AllocVec(sizeof(struct PCIXHCIUnit), MEMF_ANY|MEMF_CLEAR);
    if(unit != NULL) {
        /* PCI device is useless for us if we can't obtain it */
        if(HIDD_PCIDevice_Obtain(pciDevice, LIBBASE->library.lib_Node.ln_Name) == NULL) {
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->hc.intline);
            if(unit->hc.intline != 255) {
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Bus,          &unit->hc.bus);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Dev,          &unit->hc.dev);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Sub,          &unit->hc.sub);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&unit->hc.pcidriver);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0,  (APTR)&unit->hc.capregbase);

                unit->hc.pcidevice = pciDevice;
                unit->pcixhcibase = LIBBASE;
                unit->number = unitnum++;

                AddTail(&LIBBASE->unit_list, (struct Node *)unit);
                mybug(-1, ("[PCIXHCI] Enumerator: Host controller obtained\n"));
            } else {
                mybug(-1, ("[PCIXHCI] Enumerator: Host controller has bogus intline!\n"));
                HIDD_PCIDevice_Release(pciDevice);
                FreeVec(unit);
            }
        } else {
            mybug(-1, ("[PCIXHCI] Enumerator: Host controller could not be obtained\n"));
            FreeVec(unit);
        }
    } else {
        mybug(-1, ("\n[PCIXHCI] Enumerator: Failed to allocate unit structure!\n\n"));
    }

    AROS_USERFUNC_EXIT
}

BOOL PCIXHCI_Discover(LIBBASETYPEPTR LIBBASE) {
    mybug(0, ("[PCIXHCI] PCIXHCI_Discover: Entering function\n"));

    NEWLIST(&LIBBASE->unit_list);

    static struct TagItem tags[] = {
            { tHidd_PCI_Class,     PCI_BASE_CLASS_SERIAL },
            { tHidd_PCI_SubClass,  PCI_SUB_CLASS_USB },
            { tHidd_PCI_Interface, PCI_INTERFACE_XHCI },
            { TAG_DONE, 0UL }
    };

    struct Hook FindHook = {
            h_Entry: (IPTR (*)())GM_UNIQUENAME(Enumerator),
            h_Data:  LIBBASE,
    };

    HIDD_PCI_EnumDevices(LIBBASE->pci, &FindHook, (struct TagItem *)&tags);

    struct PCIXHCIUnit *unit;

    ForeachNode(&LIBBASE->unit_list, unit) {
        PCIXHCI_HCInit(unit);
    }

    if(!IsListEmpty(&LIBBASE->unit_list)) {
        mybug(-1, ("[PCIXHCI] Unit list is not empty\n"));
    } else {
        mybug(-1, ("[PCIXHCI] Unit list is empty\n"));
        return FALSE;
    }

    return TRUE;
}


