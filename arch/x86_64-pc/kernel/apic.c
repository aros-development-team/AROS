/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "apic.h"

/************************************************************************************************/
/************************************************************************************************
                APIC Functions used by kernel.resource from outside this file ..
 ************************************************************************************************/
/************************************************************************************************/

UBYTE core_APIC_GetNumber(struct PlatformData *pdata, IPTR __APICBase)
{
    UBYTE __APICLogicalID;
    UBYTE __APICNo;

    __APICLogicalID = __KernBootPrivate->kbp_APIC_Driver->getid(__APICBase);

    for (__APICNo = 0; __APICNo < pdata->kb_APIC_Count; __APICNo++)
    {
        if ((pdata->kb_APIC_IDMap[__APICNo] & 0xFF) == __APICLogicalID)
            return __APICNo;
    }

    return -1;
}
