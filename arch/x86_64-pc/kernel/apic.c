/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>

#include <exec/lists.h>
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <asm/io.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "apic.h"

#define D(x)

/************************************************************************************************/
/************************************************************************************************
                                    APIC Functions used by kernel.resource from outside this file ..
 ************************************************************************************************/
/************************************************************************************************/

extern const struct GenericAPIC apic_ia32_default;

static const struct GenericAPIC *probe_APIC[] =
{
    &apic_ia32_default, /* must be last */
    NULL,
};

IPTR core_APIC_Probe(struct KernBootPrivate *__KernBootPrivate)
{
    int driver_count, changed = 0;

    for (driver_count = 0; probe_APIC[driver_count]; driver_count++)
    {
	IPTR retval = probe_APIC[driver_count]->probe();

    	if (retval)
        {
	    D(bug("[Kernel] core_APICProbe: Using APIC driver '%s'\n", probe_APIC[driver_count]->name));

            __KernBootPrivate->kbp_APIC_Driver = probe_APIC[driver_count];
            return 1;
        }
    }

    D(bug("[Kernel] core_APICProbe: No suitable APIC driver found.\n"));
    return 0;
}

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
