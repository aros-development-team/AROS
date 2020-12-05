/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "hiddclass_intern.h"

#include LC_LIBDEFS_FILE

const char str_HWNamePC[] = "IBM Compatable x86 PC";
const char str_HWNameVM[] = "Virtualized x86 PC";

#undef HWAttrBase
#define HWAttrBase csd->hwAttrBase

static int init_hiddclass(LIBBASETYPEPTR lh)
{
    struct Library *OOPBase = GM_OOPBASE_FIELD(lh);
    struct  class_static_data *csd;
    struct TagItem hwTags[] =
    {
       { TAG_IGNORE,    0       },
       { TAG_DONE,      0       } 
    };
    ULONG arg = 1;
    ULONG res;

    D(bug("[HIDD] %s()\n", __func__));

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

    hwTags[0].ti_Tag = aHW_ClassName;
    asm volatile("cpuid":"+a"(arg),"=c"(res)::"%ebx", "%edx");
    if (res & 0x80000000) {
        hwTags[0].ti_Data = (IPTR)str_HWNameVM;
    }
    else {
        hwTags[0].ti_Data = (IPTR)str_HWNamePC;
    }

    D(bug("[HIDD] %s: System Type = '%s'\n", __func__, (char *)hwTags[0].ti_Data);)
    if (!OOP_NewObject(csd->rootclass, NULL, hwTags))
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
