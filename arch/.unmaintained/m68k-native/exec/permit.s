/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(void, Permit,
 
    LOCATION
 	struct ExecBase *, SysBase, 23, Exec)
 
    FUNCTION
 	This function activates the dispatcher again after a call to Permit().
 
    INPUTS
 
    RESULT
 
    NOTES
 	This function preserves all registers.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	Forbid(), Disable(), Enable()
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Permit,Exec)
	.type	AROS_SLIB_ENTRY(Permit,Exec),@function
AROS_SLIB_ENTRY(Permit,Exec):
	/* decrement nesting count and return if there are Forbid()s left */
	subq.b	#1,TDNestCnt(a6)
	bpl	end

	/* return if there are no delayed switches pending. */
	tst.b	AttnResched+1(a6)
	bpl	end

	/* if IDNestCnt is not -1 taskswitches are still forbidden */
	tst.b	IDNestCnt(a6)
	bpl	end

	/* Unset delayed switch bit and do the delayed switch */
	bclr	#7,AttnResched+1(a6)
	jsr    Switch(a6)

	/* all done. */
end:	rts

