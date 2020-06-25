/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>

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

	  KATTR_Architecture   [.G] (char *) - Name of architecture the kernel built for.

	  KATTR_ClockSource     [.G] APTR     - Used Time Source Resource, or -1 if not set.

	  KATTR_SystemLoad     [.G]
	  KATTR_CPULoad        [.G]
	  KATTR_CPULoad_END    [.G]

	  KATTR_AffinityMask   [.G]

	  KATTR_PeripheralBase [.G] IPTR     - (ARM only) peripheral IO Base address.

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

    switch (id)
    {
    case KATTR_Architecture:
	return (intptr_t)AROS_ARCHITECTURE;

    case KATTR_ClockSource:
        if (KernelBase->kb_ClockSource)
            return (intptr_t)KernelBase->kb_ClockSource;

    default:
	return -1;
    }

    AROS_LIBFUNC_EXIT
}
