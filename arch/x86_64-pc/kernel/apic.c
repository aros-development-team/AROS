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

UBYTE core_APIC_GetNumber(struct KernelBase *KernelBase, IPTR __APICBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    UBYTE __APICLogicalID;
    UBYTE __APICNo;

    __APICLogicalID = core_APIC_GetID(__APICBase);

    for (__APICNo = 0; __APICNo < KernelBase->kb_CPUCount; __APICNo++)
    {
        if ((pdata->kb_APIC_IDMap[__APICNo] & 0xFF) == __APICLogicalID)
            return __APICNo;
    }

    return -1;
}
