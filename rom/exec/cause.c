/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

/*  FUNCTION
	Schedule a software interrupt to occur. If the processor is
	currently running a user task, then the software interrupt will
	prempt the current task and run itself. From a real interrupt, it
	will queue the software interrupt for a later time.

	Software interrupts are useful from hardware interrupts if you
	wish to defer your processing down to a lower level. They can
	also be used in some special cases of device I/O. The timer.device
	and audio.device allow software interrupt driven timing and
	audio output respectively.

	Software interrupts are restricted to 5 different priority levels,
	+32, +16, 0, -16, -32.

	Software interrupts can only be scheduled once.

	The software interrupt is called with the following prototype:

	AROS_UFH3(void, YourIntCode,
	    AROS_UFHA(APTR, interruptData, A1),
	    AROS_UFHA(APTR, interruptCode, A5),
	    AROS_UFHA(struct ExecBase *, SysBase, A6))

	The interruptData is the value of the is_Data field, interruptCode
	is the value of the is_Code field - it is included for historical
	and compatibility reasons. You can ignore the value of interruptCode,
	but you must declare it.

    INPUTS
	softint     -   The interrupt you wish to schedule. When setting up
			you should set the type of the interrupt to either
			NT_INTERRUPT or NT_UNKNOWN.

    RESULT
	The software interrupt will be delivered, or queued for later
	delivery.

    NOTES
	No bounds checking on the software interrupt priority is done.
	Passing a bad priority to the system can have a strange effect.

    EXAMPLE

    BUGS
	Older versions of the Amiga operating system require that the
	software interrupts preserve the A6 register.

	Software interrupts which are added from a software interrupt of
	lower priority may not be called immediately.

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE pri;

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
    Enable();

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

AROS_UFH5(void, SoftIntDispatch,
    AROS_UFHA(ULONG, intReady, D1),
    AROS_UFHA(volatile struct Custom *, custom, A0),
    AROS_UFHA(IPTR, intData, A1),
    AROS_UFHA(ULONG_FUNC, intCode, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Interrupt *intr = NULL;
    BYTE i;

    /* disable soft ints temporarily */
    CUSTOM_ACK(INTF_SOFTINT);

    /* Don't bother if there are no software ints queued. */
    if( SysBase->SysFlags & SFF_SoftInt )
    {
	/* Clear Software interrupt pending flag. */
	SysBase->SysFlags &= ~(SFF_SoftInt);

        for(;;)
        {
            for(i=4; i>=0; i--)
            {
            	/*
            	 * This KrnCli() is needed because we could re-enter here after one handler has already
            	 * been executed. In this case interrupts have been enabled before calling the handler.
            	 */
		KrnCli();
                intr = (struct Interrupt *)RemHead(&SysBase->SoftInts[i].sh_List);

                if (intr)
                {
                    intr->is_Node.ln_Type = NT_INTERRUPT;

		    /*
		     * SoftInt handlers are called with interrupts enabled,
		     * this is how original AmigaOS(tm) works
		     */
                    KrnSti();

                    /* Call the software interrupt. */
                    AROS_UFC3(void, intr->is_Code,
                              AROS_UFCA(APTR, intr->is_Data, A1),
                              AROS_UFCA(APTR, intr->is_Code, A5),
                              AROS_UFCA(struct ExecBase *, SysBase, A6));

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
                KrnSti();
            	break;
            }
        }
    }

    /* re-enable soft ints */
    CUSTOM_ENABLE(INTB_SOFTINT);

    AROS_USERFUNC_EXIT
}
