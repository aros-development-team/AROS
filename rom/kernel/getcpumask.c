/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(uint32_t, KrnGetCPUMask,

/*  SYNOPSIS */
	AROS_LHA(uint32_t, id, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 38, Kernel)

/*  FUNCTION
	Return the affinity mask for the specified CPU number 

    INPUTS
	CPU number (as returned by KrnGetCPUNumber())

    RESULT
	CPU's affinity mask

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	KrnGetCPUNumber()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is entirely architecture-specific */
    return (1 << 0);

    AROS_LIBFUNC_EXIT
}
