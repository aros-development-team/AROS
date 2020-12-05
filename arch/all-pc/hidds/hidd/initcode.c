/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/acpica.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

#include <string.h>
#include <stdio.h>

#include "hiddclass_intern.h"

#include LC_LIBDEFS_FILE

const char str_HWNamePC[] = "IBM Compatable x86 %s";
const char str_HWNameVM[] = "Virtualized x86 %s";

const char str_HWTypePC[] = "PC";
const char str_HWTypeMobile[] = "Mobile Device";

#undef HWAttrBase
#define HWAttrBase csd->hwAttrBase

/* Parse a Battery device */
static ACPI_STATUS ACPIBatteryCallback(ACPI_HANDLE handle, ULONG nesting_level,
    void *context, void **return_value)
{
    struct  class_static_data *csd = (struct  class_static_data *)context;
    IPTR *batteryfound = (IPTR *)return_value;

    D(bug("[HIDD] %s()\n", __func__));

    *batteryfound = *batteryfound + 1;

    return AE_OK;
}

static int init_hiddclass(LIBBASETYPEPTR lh)
{
    struct Library *OOPBase = GM_OOPBASE_FIELD(lh);
    struct Library *ACPICABase;
    struct  class_static_data *csd;
    const char *sysTypeFmt = NULL, *hwType = NULL;
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
    
    /*
     * Determine what kind of hardware platform this system is..
     * TODO: When we have a separate battery hidd class, just query for those and
     * let ACPI register its found objects.
     */
    
    ACPICABase = OpenLibrary("acpica.library", 0);
    if (ACPICABase)
    {
        ACPI_STATUS status;
        IPTR batteryfound = 0;

        status = AcpiGetDevices("PNP0C0A", ACPIBatteryCallback, csd, (void **)&batteryfound);
        if (ACPI_FAILURE(status)) {
            D(bug("[HIDD] %s: No PNP0C0A battery information available\n", __func__);)
            /* not a critical failure .. */
        }
        if (batteryfound)
        {
            D(bug("[HIDD] %s: Found %u battery devices ..\n", __func__, batteryfound);)
            hwType = str_HWTypeMobile;
        }
        CloseLibrary(ACPICABase);
    }

    if (!hwType)
        hwType = str_HWTypePC;

    hwTags[0].ti_Tag = aHW_ClassName;
    asm volatile("cpuid":"+a"(arg),"=c"(res)::"%ebx", "%edx");
    if (res & 0x80000000) {
        sysTypeFmt = str_HWNameVM;
    }
    else {
        sysTypeFmt = str_HWNamePC;
    }

    hwTags[0].ti_Data = (IPTR)AllocVec(strlen(sysTypeFmt) - 1 + strlen(hwType), MEMF_CLEAR);
    sprintf((char *)hwTags[0].ti_Data, sysTypeFmt, hwType);

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
