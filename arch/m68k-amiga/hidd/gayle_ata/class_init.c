/*
    Copyright © 2013-2019, The AROS Development Team. All rights reserved
    $Id$

    Desc: A600/A1200/A4000 ATA HIDD
    Lang: English
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/storage.h>
#include <hidd/bus.h>
#include <hidd/ata.h>
#include <proto/oop.h>

#include "bus_class.h"


static void GayleATA_Cleanup(struct ataBase *base)
{
    D(bug("[ATA:Gayle] %s()\n", __func__);)
    OOP_ReleaseAttrBase(HiddAttrBase);
    OOP_ReleaseAttrBase(HiddATABusAB);
    OOP_ReleaseAttrBase(HiddBusAB);
    OOP_ReleaseAttrBase(HWAttrBase);
    CloseLibrary(base->cs_UtilityBase);
}

static int GayleATA_Init(struct ataBase *base)
{
    D(bug("[ATA:Gayle] %s()\n", __func__);)

    base->cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!base->cs_UtilityBase)
        return FALSE;

    HiddAttrBase = OOP_ObtainAttrBase(IID_Hidd);
    D(bug("[ATA:Gayle] %s: %s AB %x @ 0x%p\n", __func__, IID_Hidd, HiddAttrBase, &HiddAttrBase);)

    HiddBusAB = OOP_ObtainAttrBase(IID_Hidd_Bus);
    D(bug("[ATA:Gayle] %s: %s AB %x @ 0x%p\n", __func__, IID_Hidd_Bus, HiddBusAB, &HiddBusAB);)

    HiddATABusAB = OOP_ObtainAttrBase(IID_Hidd_ATABus);
    D(bug("[ATA:Gayle] %s: %s AB %x @ 0x%p\n", __func__, IID_Hidd_ATABus, HiddATABusAB, &HiddATABusAB);)

    HWAttrBase = OOP_ObtainAttrBase(IID_HW);
    D(bug("[ATA:Gayle] %s: %s AB %x @ 0x%p\n", __func__, IID_HW, HWAttrBase, &HWAttrBase);)

    if (!HiddAttrBase || !HiddBusAB || !HiddATABusAB || !HWAttrBase)
        return FALSE;

    HWBase = OOP_GetMethodID(IID_HW, 0);
    HiddStorageControllerBase = OOP_GetMethodID(IID_HW, 0);

    base->storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);
    if (!base->storageRoot)
        base->storageRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (!base->storageRoot)
    {
        GayleATA_Cleanup(base);

        return FALSE;
    }
    D(bug("[ATA:Gayle] %s: storage root @ 0x%p\n", __func__, base->storageRoot);)

    base->ataClass = OOP_FindClass(CLID_Hidd_ATA);
    D(bug("[ATA:Gayle] %s: %s @ 0x%p\n", __func__, CLID_Hidd_ATA, base->ataClass);)

    return TRUE;
}

static int GayleATA_Expunge(struct ataBase *base)
{
    D(bug("[ATA:Gayle] %s()\n", __func__);)

    GayleATA_Cleanup(base);

    return TRUE;
}

ADD2INITLIB(GayleATA_Init, 0)
ADD2EXPUNGELIB(GayleATA_Expunge, 0)
