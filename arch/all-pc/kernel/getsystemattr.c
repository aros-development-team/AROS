/*
    Copyright � 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include "kernel_intern.h"


/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(intptr_t, KrnGetSystemAttr,

/*  SYNOPSIS */
	AROS_LHA(uint32_t, id, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 29, Kernel)

/*  FUNCTION
	Get value of internal system attributes.
	Currently defined attributes are:

	  KATTR_Architecture [.G] (char *)        - Name of architecture the kernel built for.

	  KATTR_PeripheralBase [.G] IPTR   - IO Base address for ARM peripherals

    INPUTS
	id - ID of the attribute to get

    RESULT
	Value of the attribute

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
#if defined(__AROSEXEC_SMP__)
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData = pdata->kb_APIC;
#endif

    intptr_t retval = -1;

    if (id >= KATTR_CPULoad && id < KATTR_CPULoad_END)
    {
        id -= KATTR_CPULoad;
        if (id < apicData->apic_count)
            retval = (100 * (apicData->cores[id].cpu_Load >> 16)) >> 16;
    }
    else if (id == KATTR_Architecture)
    {
        retval = (intptr_t)AROS_ARCHITECTURE;
    }
    
    return retval;

    AROS_LIBFUNC_EXIT
}
