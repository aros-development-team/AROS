/*
    Copyright © 2013, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

#include <aros/symbolsets.h>
#include <hidd/ata.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "bus_class.h"

static CONST_STRPTR attrBaseIDs[] =
{
    IID_Hidd_PCIDevice,
    IID_Hidd_PCIDriver,
    IID_Hidd,
    IID_Hidd_ATABus,
    NULL
};

#define AB_MANDATORY 2

static int pciata_init(struct ataBase *base)
{
    base->cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!base->cs_UtilityBase)
        return FALSE;

    base->cs_KernelBase = OpenResource("kernel.resource");
    if (!base->cs_KernelBase)
        return FALSE;

    /*
     * We handle also legacy ISA devices, so we can work without PCI subsystem.
     * Because of this, we do not obtain PCI bases here. We do it later, in device
     * discovery code.
     */
    if (OOP_ObtainAttrBasesArray(&base->hiddAttrBase, &attrBaseIDs[AB_MANDATORY]))
        return FALSE;

    return TRUE;
}

static int pciata_expunge(struct ataBase *base)
{
    /* Release all attribute bases */
    OOP_ReleaseAttrBasesArray(&base->hiddAttrBase, attrBaseIDs);

    if (base->cs_UtilityBase)
        CloseLibrary(base->cs_UtilityBase);

    return TRUE;
}

ADD2INITLIB(pciata_init, 0)
ADD2EXPUNGELIB(pciata_expunge, 0)
