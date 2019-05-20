/*
    Copyright © 2019, The AROS Development Team. All rights reserved
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <hidd/scsi.h>
#include <hidd/storage.h>
#include <hidd/hidd.h>

#include "wd33c93_intern.h"

static CONST_STRPTR attrBaseIDs[] =
{
    IID_Hidd,
    IID_Hidd_Bus,
    IID_Hidd_SCSIBus,
    IID_HW,
    NULL
};

#if defined(__OOP_NOMETHODBASES__)
static CONST_STRPTR const methBaseIDs[] =
{
    IID_HW,
    IID_Hidd_StorageController,
    NULL
};
#endif

static int wd33c93_HiddInit(struct scsiwd33c93Base *base)
{
    D(bug("[SCSI:WD33C93] %s()\n", __PRETTY_FUNCTION__));

    base->cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!base->cs_UtilityBase)
        return FALSE;

    base->cs_KernelBase = OpenResource("kernel.resource");
    if (!base->cs_KernelBase)
        return FALSE;

    if (OOP_ObtainAttrBasesArray(&base->hiddAttrBase, attrBaseIDs))
        return FALSE;

    base->storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);
    if (!base->storageRoot)
        base->storageRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (!base->storageRoot)
    {
        OOP_ReleaseAttrBasesArray(&base->hiddAttrBase, attrBaseIDs);
        return FALSE;
    }
    D(bug("[SCSI:WD33C93] %s: storage root @ 0x%p\n", __PRETTY_FUNCTION__, base->storageRoot);)

    if ((base->scsiClass = OOP_FindClass(CLID_Hidd_SCSI)) == NULL)
    {
        OOP_ReleaseAttrBasesArray(&base->hiddAttrBase, attrBaseIDs);
        return FALSE; 
    }
    D(
      bug("[SCSI:WD33C93] %s: Base %s Class @ 0x%p\n", __PRETTY_FUNCTION__, CLID_Hidd_SCSI, base->scsiClass);
      bug("[SCSI:WD33C93] %s: WD33C93 %s Class @ 0x%p\n", __PRETTY_FUNCTION__, CLID_Hidd_SCSIBus, base->busClass);
    )

#if defined(__OOP_NOMETHODBASES__)
    if (OOP_ObtainMethodBasesArray(&base->HWMethodBase, methBaseIDs))
    {
        bug("[SCSI:WD33C93] %s: Failed to obtain MethodBases!\n", __PRETTY_FUNCTION__);
        bug("[SCSI:WD33C93] %s:     %s = %p\n", __PRETTY_FUNCTION__, methBaseIDs[0], base->HWMethodBase);
        bug("[SCSI:WD33C93] %s:     %s = %p\n", __PRETTY_FUNCTION__, methBaseIDs[1], base->HiddSCMethodBase);
        OOP_ReleaseAttrBasesArray(&base->hiddAttrBase, attrBaseIDs);
        return FALSE;
    }
#endif

    return TRUE;
}

static int wd33c93_expunge(struct scsiwd33c93Base *base)
{
    /* Release all attribute bases */
    OOP_ReleaseAttrBasesArray(&base->hiddAttrBase, attrBaseIDs);

    if (base->cs_UtilityBase)
        CloseLibrary(base->cs_UtilityBase);

    return TRUE;
}

ADD2INITLIB(wd33c93_HiddInit, 0)
ADD2EXPUNGELIB(wd33c93_expunge, 0)
