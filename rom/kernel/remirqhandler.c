/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(void, KrnRemIRQHandler,

/*  SYNOPSIS */
        AROS_LHA(void *, handle, A0),

/*  LOCATION */
         struct KernelBase *, KernelBase, 8, Kernel)

/*  FUNCTION
	Remove previously installed hardware IRQ handler

    INPUTS
	handle - an opaque handler returned by KrnAddIRQHandler()
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
    uint8_t irq = h->in_nr;

    if (h && (h->in_type == it_interrupt))
    {
        (void)goSuper();

        Disable();
        REMOVE(h);
        if (IsListEmpty(&KernelBase->kb_Interrupts[irq]))
        {
        	ictl_disable_irq(irq, KernelBase);
        }
        Enable();

        krnFreeIntrNode(h);

        goUser();
    }

    AROS_LIBFUNC_EXIT
}
