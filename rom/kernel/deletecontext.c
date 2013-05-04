/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <proto/exec.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1(void, KrnDeleteContext,

/*  SYNOPSIS */
	AROS_LHA(void *, context, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 19, Kernel)

/*  FUNCTION
	Free CPU context storage area

    INPUTS
    	context - a pointer to a CPU context storage previously allocated using
    		  KrnCreateContext()

    RESULT
    	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    	KrnCreateContext()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * This is actually a pair to krnAllocCPUContext().
     * Needs to be reimplemented on a per-architecture basis if needed.
     */
    FreeMem(context, KernelBase->kb_ContextSize);

    AROS_LIBFUNC_EXIT
}
