/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_objects.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void *, KrnCreateContext,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 18, Kernel)

/*  FUNCTION
	Allocate and initialize CPU context storage area.

    INPUTS
	None.

    RESULT
	A pointer to a CPU context storage area.

    NOTES
	CPU context storage is considered private and accessible
	only from within supevisor mode.

    EXAMPLE

    BUGS

    SEE ALSO
    	KrnDeleteContext()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct AROSCPUContext *ctx;
    cpumode_t mode = goSuper();

    /* Our context is accessible only in supervisor mode */
    ctx = krnAllocCPUContext();

    /* Initialize the storage if needed */
#ifdef PREPARE_INITIAL_CONTEXT
    if (ctx)
	PREPARE_INITIAL_CONTEXT(ctx);
#endif

    goBack(mode);

    return ctx;

    AROS_LIBFUNC_EXIT
}
