/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "hiddclass_intern.h"

#include LC_LIBDEFS_FILE

static int init_hiddclass(LIBBASETYPEPTR lh)
{
    struct Library *OOPBase = GM_OOPBASE_FIELD(lh);
    struct  class_static_data *csd;

    EnterFunc(bug("HIDD::Init()\n"));

    csd = &lh->hd_csd;
    csd->cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!csd->cs_UtilityBase)
        return FALSE;

    csd->MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 8192, 4096);
    if (!csd->MemPool)
        return FALSE;
        
    csd->hiddAttrBase = OOP_ObtainAttrBase(IID_Hidd);
    if (!csd->hiddAttrBase)
        return FALSE;
    csd->hwAttrBase = OOP_ObtainAttrBase(IID_HW);
    if (!csd->hwAttrBase)
        return FALSE;

    csd->hwMethodBase = OOP_GetMethodID(IID_HW, 0);

    if (!OOP_NewObject(csd->rootclass, NULL, NULL))
        return FALSE;

    ReturnInt("HIDD::Init", ULONG, TRUE);
}

static int free_hiddclass(LIBBASETYPEPTR lh)
{
    struct Library *OOPBase = GM_OOPBASE_FIELD(lh);
    struct class_static_data *csd = &lh->hd_csd;

    EnterFunc(bug("HIDD::Free()\n"));

    /* We do not dispose csd->hwroot because we will never be expunged */

    if (csd->hwAttrBase)
        OOP_ReleaseAttrBase(IID_HW);
    if (csd->hiddAttrBase)
        OOP_ReleaseAttrBase(IID_Hidd);

    DeletePool(csd->MemPool);
    CloseLibrary(csd->cs_UtilityBase);

    ReturnInt("HIDD::Free", ULONG, TRUE);
}

ADD2INITLIB(init_hiddclass, 0)
ADD2EXPUNGELIB(free_hiddclass, 0)
