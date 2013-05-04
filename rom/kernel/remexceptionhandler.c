/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_debug.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define D(x)

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1(void, KrnRemExceptionHandler,

/*  SYNOPSIS */
        AROS_LHA(void *, handle, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 15, Kernel)

/*  FUNCTION
	Remove previously installed CPU exception handler

    INPUTS
	handle - an opaque handler returned by KrnAddExceptionHandler()
	         function

    RESULT
	None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntrNode *h = handle;

    if (h && (h->in_type == it_exception))
    {
        (void)goSuper();

        Disable();
        REMOVE(h);
        Enable();

        krnFreeIntrNode(h);

        goUser();
    }

    AROS_LIBFUNC_EXIT
}
