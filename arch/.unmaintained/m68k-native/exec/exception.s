/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 	AROS_LH0(void, Exception,
 
    LOCATION
 	struct ExecBase *, SysBase, 11, Exec)
 
    FUNCTION
 	Exception handler. This function is called by the dispatcher if a
 	exception has to be delivered. It is called in Disable()d state
 	(atomically) so that all Signals are still unchanged.
 	TF_EXCEPT is still set and must be reset by this routine.
 
    INPUTS
 
    RESULT
 
    NOTES
 	This function has a context on its own and does not need to preserve
 	any registers.
 
 	Internal exec function.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Exception,Exec)
	.type	AROS_SLIB_ENTRY(Exception,Exec),@function
AROS_SLIB_ENTRY(Exception,Exec):
	/* First clear task exception bit. */
	move.l	ThisTask(a6),a2
	bclr	#TB_EXCEPT,tc_Flags(a2)

	/* If the exception is raised out of a Wait()
	   IDNestCnt may be almost everything.
	   Store nesting level and set it to a
	   defined value 1 beyond -1.
	*/
excusr: move.b	IDNestCnt(a6),d2
	clr.b	IDNestCnt(a6)

exloop: /* get signals causing the exception (do nothing if there are none) */
	move.l	tc_SigExcept(a2),d0
	and.l	tc_SigRecvd(a2),d0
	beq	excend

	/* disable the signals */
	eor.l	d0,tc_SigExcept(a2)
	eor.l	d0,tc_SigRecvd(a2)

	/* call the exception vector with interrupts enabled */
	move.l	tc_ExceptData(a2),a1
	move.l	tc_ExceptCode(a2),a0
	jsr	Enable(a6)
	jsr	(a0)
	jsr	Disable(a6)

	/* reenable signals and look again */
	or.l	d0,tc_SigExcept(a2)
	bra	exloop

	/* restore state of Disable() and return */
excend: move.b	d2,IDNestCnt(a6)
	rts

