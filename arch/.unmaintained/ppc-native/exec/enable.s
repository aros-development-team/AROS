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
	subr
	push	scr

	lbz	scr,IDNestCnt(base)
	subic.	scr,scr,1
	stw	scr,IDNestCnt(base)
	bge	end

	lwz	arg0,_enab(0)
	jsrlvo	Supervisor,base
	/* we should come back here from _disab */
	/* enable Amiga chipset interrupts */
	li	scr,INTEN+SET
	stw	scr,INTENA

	/* return if there are no delayed switches pending. */
	lwz	scr,AttnResched+1(base)
	cmpdi	scr,0
	beq	end

	/* if TDNestCnt is not -1 taskswitches are still forbidden */
	lwz	scr,TDNestCnt(base)
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

_enab:
	mfmsr	p1
	ori	p1,p1,0x8000
	isync
	mtmsr
	sync
	rfi
