/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(void, Enable,
 
    LOCATION
 	struct ExecBase *, SysBase, 21, Exec)
 
    FUNCTION
 	This function reenables the delivery of interrupts after a call to
 	Disable().
 
    INPUTS
 
    RESULT
 
    NOTES
 	This function preserves all registers.
 
 	This function may be used from interrupts.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	Forbid(), Permit(), Disable()
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	INTENA	    =	0xdff09a
	INTEN	    =	0x4000
	SET	    =	0x8000

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Enable,Exec)
	.type	AROS_SLIB_ENTRY(Enable,Exec),@function
AROS_SLIB_ENTRY(Enable,Exec):
	/* decrement nesting count and return if there are Disable()s left */
	subq.b	#1,IDNestCnt(a6)
	bpl	end

	/* reenable interrupts */
	move.w	#INTEN+SET,INTENA

	/* return if there are no delayed switches pending. */
	tst.b	AttnResched+1(a6)
	bpl	end

	/* if TDNestCnt is not -1 taskswitches are still forbidden */
	tst.b	TDNestCnt(a6)
	bpl	end

	/* Unset delayed switch bit and do the delayed switch */
	bclr	#7,0x12b(a6)
	jsr	Switch(a6)

	/* all done. */
end:	rts

