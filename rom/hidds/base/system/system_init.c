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

#include "system_intern.h"

#include LC_LIBDEFS_FILE

static int Systsem_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hsi_csd;
    struct Library *OOPBase = csd->cs_OOPBase;

    D(bug("[HiddSystsem] %s()\n", __PRETTY_FUNCTION__));

    OOP_Object *hwroot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    csd->hwAttrBase = OOP_ObtainAttrBase(IID_HW);

    if (HW_AddDriver(hwroot, csd->oopclass, NULL))
    {
        D(bug("[HiddSystsem] %s: initialised\n", __PRETTY_FUNCTION__));
        return TRUE;
    }
    D(bug("[HiddSystsem] %s: failed\n", __PRETTY_FUNCTION__));
    
    return FALSE;
}

static int Systsem_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(struct class_static_data *csd = &LIBBASE->hsi_csd;)
#if (0)
    struct Library *OOPBase = csd->cs_OOPBase;
#endif
    
    D(bug("[HiddSystsem] %s(csd=%p)\n", __PRETTY_FUNCTION__, csd));
    
    return TRUE;
}

ADD2INITLIB(Systsem_Init, -2)
ADD2EXPUNGELIB(Systsem_Expunge, -2)
