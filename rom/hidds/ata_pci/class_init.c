/*
    Copyright © 2013-2018, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <hidd/ata.h>
#include <hidd/storage.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>

#include "bus_class.h"

static CONST_STRPTR attrBaseIDs[] =
{
    IID_Hidd_PCIDevice,
    IID_Hidd_PCIDriver,
    IID_Hidd,
    IID_Hidd_ATABus,
    IID_HW,
    NULL
};

#define AB_MANDATORY 2

#if defined(__OOP_NOMETHODBASES__)
static CONST_STRPTR const methBaseIDs[] =
{
    IID_HW,
    IID_Hidd_StorageController,
    NULL
};
#endif

static int pciata_init(struct atapciBase *base)
{
    D(bug("[ATA:PCI] %s()\n", __PRETTY_FUNCTION__));

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

    base->storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);
    if (!base->storageRoot)
        base->storageRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (!base->storageRoot)
    {
        OOP_ReleaseAttrBasesArray(&base->hiddAttrBase, &attrBaseIDs[AB_MANDATORY]);
        return FALSE;
    }
    D(bug("[ATA:PCI] %s: storage root @ 0x%p\n", __PRETTY_FUNCTION__, base->storageRoot);)

    if ((base->ataClass = OOP_FindClass(CLID_Hidd_ATA)) == NULL)
    {
        OOP_ReleaseAttrBasesArray(&base->hiddAttrBase, &attrBaseIDs[AB_MANDATORY]);
        return FALSE; 
    }
    D(
      bug("[ATA:PCI] %s: Base %s Class @ 0x%p\n", __PRETTY_FUNCTION__, CLID_Hidd_ATA, base->ataClass);
      bug("[ATA:PCI] %s: PCI %s Class @ 0x%p\n", __PRETTY_FUNCTION__, CLID_Hidd_ATABus, base->busClass);
    )

#if defined(__OOP_NOMETHODBASES__)
    if (OOP_ObtainMethodBasesArray(&base->HWMethodBase, methBaseIDs))
    {
        bug("[ATA:PCI] %s: Failed to obtain MethodBases!\n", __PRETTY_FUNCTION__);
        bug("[ATA:PCI] %s:     %s = %p\n", __PRETTY_FUNCTION__, methBaseIDs[0], base->HWMethodBase);
        bug("[ATA:PCI] %s:     %s = %p\n", __PRETTY_FUNCTION__, methBaseIDs[1], base->HiddSCMethodBase);
        OOP_ReleaseAttrBasesArray(&base->hiddAttrBase, attrBaseIDs);
        return FALSE;
    }
#endif

    return TRUE;
}

static int pciata_expunge(struct atapciBase *base)
{
    /* Release all attribute bases */
    OOP_ReleaseAttrBasesArray(&base->hiddAttrBase, &attrBaseIDs[AB_MANDATORY]);

    if (base->cs_UtilityBase)
        CloseLibrary(base->cs_UtilityBase);

    return TRUE;
}

ADD2INITLIB(pciata_init, 0)
ADD2EXPUNGELIB(pciata_expunge, 0)
