/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Cause() - Cause a software interrupt.
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/asmcall.h>
#include <hardware/custom.h>

/* Change this to <exec_intern.h> if you move this file... */
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

    /* Check to ensure that this node is not already in a list. */
    if( softint->is_Node.ln_Type != NT_SOFTINT )
    {
	/* Scale the priority down to a number between 0 and 4 inclusive
	   We can use that to index into exec's software interrupt lists. */
	pri = ((softint->is_Node.ln_Pri & 0xF0) + 0x20) >> 4;

	/* We are accessing an Exec list, protect ourselves. */
	Disable();
	AddTail((struct List *)&SysBase->SoftInts[pri],
		(struct Node *)softint);
	softint->is_Node.ln_Type = NT_SOFTINT;
	SysBase->SysFlags |= SFF_SoftInt;
	Enable();

	/* We now cause a software interrupt. Pretty hard here... */
#ifndef __CXREF__
#error The '$(KERNEL)' interrupt implementation has not been completed.
#endif
    }

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

    This procedure could be more efficient, and it has to be implemented
    in the kernel.
*/

AROS_UFH5(ULONG, SoftIntDispatch,
    AROS_UFHA(ULONG, intReady, D1),
    AROS_UFHA(struct Custom *, custom, A0),
    AROS_UFHA(IPTR, intData, A1),
    AROS_UFHA(ULONG_FUNC, intCode, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Interrupt *intr;
    UBYTE i;
    ULONG res;

    /*
	Hmm, we have to disable software interrupts ONLY. Do NOT disable
	any other kind of interrupt. This is very hard to do here however.

	The #error line above will trap however.
    */

    /* Don't bother if there are no software ints queued. */
    if( SysBase->SysFlags & SFF_SoftInt )
    {
	/* Clear Software interrupt pending flag. */
	SysBase->SysFlags &= ~(SFF_SoftInt);

	for(i=0; i < 4; i++)
	{
	    /* There is a possible problem here with list access */
	    while( (intr = RemHead((struct List *)&SysBase->SoftInts[i])) )
	    {
		intr->is_Node.ln_Type = NT_INTERRUPT;

		/* Call the software interrupt. */
		AROS_UFC3(void, intr->is_Code,
		    AROS_UFCA(APTR, intr->is_Data, A1),
		    AROS_UFCA(ULONG_FUNC, intr->is_Code, A5),
		    AROS_UFCA(struct ExecBase *, SysBase, A6));
	    }
	}
    }

    /* We now re-enable software interrupts. But we can't do it here. */
    AROS_USERFUNC_EXIT
}

