/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <proto/exec.h>

#include <inttypes.h>

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

        AROS_LH4(void *, KrnAddExceptionHandler,

/*  SYNOPSIS */
        AROS_LHA(uint8_t, num, D0),
        AROS_LHA(exhandler_t *, handler, A0),
        AROS_LHA(void *, handlerData, A1),
        AROS_LHA(void *, handlerData2, A2),

/*  LOCATION */
        struct KernelBase *, KernelBase, 14, Kernel)

/*  FUNCTION
	Add a raw CPU exception handler to the chain of handlers.

    INPUTS
	num          - CPU-specific exception number
	handler      - Pointer to a handler function
	handlerData,
	handlerData2 - User-defined data which is passed to the
		       handler.
	
	  Handler function uses a C calling convention and must be
	  declared as follows:
	  
	  int ExceptionHandler(void *ctx, void *handlerData, void *handlerData2)
	    
	  handlerData and handlerData2 will be values passed to the
	  KrnAddExceptionHandler() function. ctx is a CPU context handle.
	  Consider this parameter private and reserved for now.

	  Exception handler should return nonzero value if it processes the
	  exception and wants to continue program execution. Otherwise it should
	  return zero. If all exception handlers in the chain return zero, the
	  exception will be passed on to exec.library trap handler pointed to
	  by tc_TrapCode field of task structure.

    RESULT
	An opaque handle that can be used for handler removal or NULL in case
	of failure (like unsupported exception number).

    NOTES
	The specification of this function is preliminary and subject to change.
	Consider it private for now.

    EXAMPLE

    BUGS

    SEE ALSO
	KrnRemExceptionHandler()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntrNode *handle = NULL;
    D(bug("[KRN] KrnAddExceptionHandler(%02x, %012p, %012p, %012p):\n", num, handler, handlerData, handlerData2));

    if (num < EXCEPTIONS_COUNT)
    {
        /* Go to supervisor mode */
        (void)goSuper();

	/* Allocate protected memory, accessible only in supervisor mode */
        handle = krnAllocIntrNode();
        D(bug("[KRN] handle=%012p\n", handle));

        if (handle)
        {
            handle->in_Handler = handler;
            handle->in_HandlerData = handlerData;
            handle->in_HandlerData2 = handlerData2;
            handle->in_type = it_exception;
            handle->in_nr = num;

            Disable();
            ADDHEAD(&KernelBase->kb_Exceptions[num], &handle->in_Node);
            Enable();
        }

        goUser();
    }

    return handle;

    AROS_LIBFUNC_EXIT
}

/* Run exception handlers and accumulate return value */
int krnRunExceptionHandlers(struct KernelBase *KernelBase, uint8_t exception, void *ctx)
{
    struct IntrNode *in, *in2;
    int ret = 0;

    /* We can be called really early. Protect against this. */
    if (!KernelBase)
    	return 0;

    ForeachNodeSafe(&KernelBase->kb_Exceptions[exception], in, in2)
    {
	exhandler_t h = in->in_Handler;

        if (h)
            ret |= h(ctx, in->in_HandlerData, in->in_HandlerData2);
    }

    return ret;
}
