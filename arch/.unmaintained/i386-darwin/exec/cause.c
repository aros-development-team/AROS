/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386unix version of Cause().
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <hardware/custom.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/kernel.h>
#include <proto/arossupport.h>

#include <exec_intern.h>

AROS_LH1(void, Cause,
    AROS_LHA(struct Interrupt *, softint, A1),
    struct ExecBase *, SysBase, 30, Exec)
{
    AROS_LIBFUNC_INIT

    UBYTE pri;

	KRNWireImpl(SoftCause);

    /* Check to ensure that this node is not already in a list. */
    if( softint->is_Node.ln_Type != NT_SOFTINT )
    {
	/* Scale the priority down to a number between 0 and 4 inclusive
	   We can use that to index into exec's software interrupt lists. */
	pri = (softint->is_Node.ln_Pri + 0x20) >> 4;

	/* We are accessing an Exec list, protect ourselves. */
	Disable();
	AddTail((struct List *)&SysBase->SoftInts[pri], (struct Node *)softint);
	softint->is_Node.ln_Type = NT_SOFTINT;
	SysBase->SysFlags |= SFF_SoftInt;
	Enable();

	  /* We now cause a software interrupt. */
	  CALLHOOKPKT(krnSoftCauseImpl,0,0);
    }

    AROS_LIBFUNC_EXIT
} /* Cause() */

/*
    This is the dispatcher for software interrupts. We go through the
    lists and remove the interrupts before calling them. Because we
    can be interrupted we should not cache the next pointer, but
    retreive it from ExecBase each time.

    Note: All these arguments are passed to the structure, so you must
    at least declare all of them. You do not however have to use any
    of them (although that is recommended).

    This procedure could be more efficient, and it has to be implemented
    in the kernel.
*/

AROS_UFH5(void, SoftIntDispatch,
    AROS_UFHA(ULONG, intReady, D1),
    AROS_UFHA(struct Custom *, custom, A0),
    AROS_UFHA(IPTR, intData, A1),
    AROS_UFHA(IPTR, intCode, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Interrupt *intr;
    UBYTE i;

	KRNWireImpl(SoftEnable);
	KRNWireImpl(SoftDisable);
    
    if( SysBase->SysFlags & SFF_SoftInt )
    {
	/* Disable software interrupts */
	CALLHOOKPKT(krnSoftDisableImpl,0,0);
	  
	/* Clear the Software interrupt pending flag. */
	SysBase->SysFlags &= ~(SFF_SoftInt);

	for(i=0; i < 4; i++)
	{
	    while( (intr = (struct Interrupt *)RemHead((struct List *)&SysBase->SoftInts[i])) )
	    {
		intr->is_Node.ln_Type = NT_INTERRUPT;

		/* Call the software interrupt. */
		AROS_UFC3(void, intr->is_Code,
		    AROS_UFCA(APTR, intr->is_Data, A1),
		    AROS_UFCA(APTR, intr->is_Code, A5),
		    AROS_UFCA(struct ExecBase *, SysBase, A6));
	    }
	}

	/* We now re-enable software interrupts. */
	  CALLHOOKPKT(krnSoftEnableImpl,0,0);
    }

    AROS_USERFUNC_EXIT
}
