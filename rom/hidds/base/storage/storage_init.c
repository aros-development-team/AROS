/*
    Copyright © 2015-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>

#include "storage_intern.h"

#include LC_LIBDEFS_FILE

static int Storage_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hsi_csd;
    struct Library *OOPBase = csd->cs_OOPBase;

    D(bug("[HiddStorage] %s()\n", __func__));

    OOP_Object *hwroot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    csd->hwAttrBase = OOP_ObtainAttrBase(IID_HW);

    if (HW_AddDriver(hwroot, csd->oopclass, NULL))
    {
        D(bug("[HiddStorage] %s: initialised\n", __func__));
        return TRUE;
    }
    D(bug("[HiddStorage] %s: failed\n", __func__));
    
    return FALSE;
}

static int Storage_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(struct class_static_data *csd = &LIBBASE->hsi_csd;)
#if (0)
    struct Library *OOPBase = csd->cs_OOPBase;
#endif
    D(bug("[HiddStorage] %s(csd=%p)\n", __func__, csd));
    
    return TRUE;
}

ADD2INITLIB(Storage_Init, -2)
ADD2EXPUNGELIB(Storage_Expunge, -2)
