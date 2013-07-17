/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/config.h>

#include <kernel_base.h>
#include <kernel_mm.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(void, KrnInitMemory,

/*  SYNOPSIS */
	AROS_LHA(struct MemHeader *, mh, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 31, Kernel)

/*  FUNCTION
	Initialize kernel memory management on a given memory region

    INPUTS
    	mh - Address of a filled in structure describing the region.

    RESULT
    	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if USE_MMU
    if (!KernelBase->kb_PageSize)
	return;

    /* Initialize the MemHeader */
    mm_Init(mh, KernelBase->kb_PageSize);
#endif

    AROS_LIBFUNC_EXIT
}
