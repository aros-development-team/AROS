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
	subr
	push	scr

	/* decrement nesting count and return if there are Forbid()s left */
	lbz	scr,TDNestCnt(base)
	subic.	scr,scr,1
	stw	scr,TDNestCnt(base)
	bge	end

	/* return if there are no delayed switches pending. */
	lwz	scr,AttnResched+1(base)
	cmpdi	scr,0
	beq	end

	/* if IDNestCnt is not -1 taskswitches are still forbidden */
	lwz	scr,IDNestCnt(base)
	cmpdi	scr,0
	beq	end

	/* Unset delayed switch bit and do the delayed switch */
	lwz	scr,0x12b(base)
	andi.	scr,scr,0x7f
	sth	scr,0x12b(base)
	jsrlvo	Switch,base

	/* all done. */

end:	pop	scr
	rts

