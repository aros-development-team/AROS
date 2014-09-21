/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Cause() - Cause a software interrupt.
    Lang: english
*/

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/kernel.h>

#include "chipset.h"
#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <exec/interrupts.h>
#include <proto/exec.h>

	AROS_LH1(void, Cause,

/*  SYNOPSIS */
	AROS_LHA(struct Interrupt *, softint, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 30, Exec)

{
    AROS_LIBFUNC_INIT

    UBYTE pri;

    bug("[exec] Cause: Disabling interupts\n");
    Disable();

    /* Check to ensure that this node is not already in a list. */
    if (softint->is_Node.ln_Type != NT_SOFTINT)
    {
        /* Scale the priority down to a number between 0 and 4 inclusive
        We can use that to index into exec's software interrupt lists. */
        pri = (softint->is_Node.ln_Pri + 0x20)>>4;

        /* We are accessing an Exec list, protect ourselves. */
        ADDTAIL(&SysBase->SoftInts[pri].sh_List, &softint->is_Node);
        softint->is_Node.ln_Type = NT_SOFTINT;

        /* Signal pending software interrupt condition */
        SysBase->SysFlags |= SFF_SoftInt;

	/*
	 * Quick soft int request. For optimal performance m68k-amiga
	 * Enable() does not do any extra SFF_SoftInt checks
	 */
	CUSTOM_CAUSE(INTF_SOFTINT);
        /*
         * If we are in usermode the software interrupt will end up being triggered
         * in Enable(). On Amiga hardware this happens because a hardware interrupt
         * was queued. On other machines Enable() will simulate this behavior,
         * looking at SFF_SoftInt flag.
         */
    }
    bug("[exec] Cause: Enabling interupts\n");
    Enable();
    bug("[exec] Cause: done...\n");
    AROS_LIBFUNC_EXIT
} /* Cause() */

/*
    This is the dispatcher for software interrupts. We go through the
    lists and remove the interrupts before calling them. Because we
    can be interrupted we should not cache the next pointer, but
    retreive it from ExecBase each time.

    Note: All these arguments are passed to the function, so you must
    at least declare all of them. You do not however have to use any
    of them (although that is recommended).

    This procedure could be more efficient.
*/

AROS_INTH0(SoftIntDispatch)
{
    AROS_INTFUNC_INIT

    struct Interrupt *intr = NULL;
    BYTE i;

    bug("[exec] SoftInt: dispatching...\n");

    /* disable soft ints temporarily */
    CUSTOM_ACK(INTF_SOFTINT);

    /* Don't bother if there are no software ints queued. */
    if( SysBase->SysFlags & SFF_SoftInt )
    {
	/* Clear Software interrupt pending flag. */
	SysBase->SysFlags &= ~(SFF_SoftInt);

        for(;;)
        {
            /*
             * This KrnCli() is needed because we could re-enter here after one handler has already
             * been executed. In this case interrupts have been enabled before calling the handler.
             */
#if (0)
            KrnCli();
#endif
            for(i=4; i>=0; i--)
            {
                intr = (struct Interrupt *)RemHead(&SysBase->SoftInts[i].sh_List);

                if (intr)
                {
                    intr->is_Node.ln_Type = NT_INTERRUPT;

                    bug("[exec] SoftInt: interrupt @ 0x%p [code @ 0x%p, data @ 0x%p]\n", intr, intr->is_Code, intr->is_Data);

		    /*
		     * SoftInt handlers are called with interrupts enabled,
		     * this is how original AmigaOS(tm) works
		     */
#if (0)
                    KrnSti();
#endif
                    bug("[exec] SoftInt: calling soft interrupt\n");

                    /* Call the software interrupt. */
                    AROS_INTC1(intr->is_Code, intr->is_Data);

                    /* Get out and start loop *all* over again *from scratch*! */
                    break;
                }
            }
            if (!intr)
            {
            	/*
            	 * We executed KrnCli() and attempted to fetch a request from the list,
            	 * but the list was empty.
            	 * We are going to exit, but before this we need to re-enable
            	 * interrupts. Otherwise we exit this vector with disabled interrupts,
            	 * screwing things up.
            	 */
#if (0)
                KrnSti();
#endif
            	break;
            }
        }

    }

    /* re-enable soft ints */
    CUSTOM_ENABLE(INTB_SOFTINT);

    bug("[exec] SoftInt: done...\n");
    
    return FALSE;

    AROS_INTFUNC_EXIT
}
