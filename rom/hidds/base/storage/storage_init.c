/*
    Copyright © 2015-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>

#include "storage_intern.h"

#include LC_LIBDEFS_FILE

#undef CSD
#define CSD(x) csd

static int Storage_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hsi_csd;

    D(bug("[HiddStorage] %s(csd=%p)\n", __func__, csd));

    OOP_Object *hwroot = OOP_NewObject(NULL, CLID_HW_Root, NULL);

    if (hwroot)
    {
        D(bug("[HiddStorage] %s: hwroot @ 0x%p\n", __func__, hwroot));
        csd->hwAttrBase = OOP_ObtainAttrBase(IID_HW);
        csd->hwMethodBase = OOP_GetMethodID(IID_HW, 0);
        csd->hiddSCMethodBase = OOP_GetMethodID(IID_Hidd_StorageController, 0);

        NEWLIST(&csd->cs_IDs);

        csd->cs_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
        if (csd->cs_MemPool == NULL)
            return FALSE;

        D(bug("[HiddStorage] %s: MemPool @ %p\n", __PRETTY_FUNCTION__, csd->cs_MemPool));

        if (HW_AddDriver(hwroot, csd->storageClass, NULL))
        {
            D(bug("[HiddStorage] %s: initialised\n", __func__));
            return TRUE;
        }
    }
    D(bug("[HiddStorage] %s: failed\n", __func__));

    return FALSE;
}

static int Storage_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(struct class_static_data *csd = &LIBBASE->hsi_csd;)

    D(bug("[HiddStorage] %s(csd=%p)\n", __func__, csd));

    return TRUE;
}

ADD2INITLIB(Storage_Init, -2)
ADD2EXPUNGELIB(Storage_Expunge, -2)
