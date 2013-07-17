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

        AROS_LH3(void, KrnModifyIRQHandler,

/*  SYNOPSIS */
        AROS_LHA(void *, handle, A0),
        AROS_LHA(void *, handlerData, A1),
        AROS_LHA(void *, handlerData2, A2),

/*  LOCATION */
        struct KernelBase *, KernelBase, 38, Kernel)

/*  FUNCTION
	Modify the data passed to a raw hardware IRQ handler.

    INPUTS
	handle       - Existing handle
	handlerData,
	handlerData2 - User-defined data which is passed to the
		       handler.
	
    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	KrnAddIRQHandler(), KrnRemIRQHandler()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntrNode *intrhandle = (struct IntrNode *)handle;

    D(bug("[KRN] KrnModifyIRQHandler(%012p, %012p, %012p):\n", handle, handlerData, handlerData2));

    if (handle)
    {
        Disable();

        intrhandle->in_HandlerData = handlerData;
        intrhandle->in_HandlerData2 = handlerData2;

        Enable();
    }

    AROS_LIBFUNC_EXIT
}
