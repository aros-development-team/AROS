/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH1(ULONG, Supervisor,
 
    SYNOPSIS
 	AROS_LHA(ULONG_FUNC, userFunction, A5),
 
    LOCATION
 	struct ExecBase *, SysBase, 5, Exec)
 
    FUNCTION
 	Call a routine in supervisor mode. This routine runs on the
 	supervisor stack and must end with a "rte". No registers are spilled,
 	i.e. Supervisor() effectively works like a function call.
 
    INPUTS
 	userFunction - address of the function to be called.
 
    RESULT
 	whatever the function left in the registers
 
    NOTES
 	This function is CPU dependant.
 
    EXAMPLE
 
    BUGS
 	Context switches that happen during the duration of this call are lost.
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "../machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Supervisor,Exec)

AROS_SLIB_ENTRY(Supervisor,Exec):
	/* I am cheating my way through this routine, because ixemul traps all
	   exceptions. We would never be able to finish this function.
	   I need to change one register, though, because I need to get this
	   Amiga's original ExecBase. I will use register a4 for this. I also
	   assume the original Supervisor() function does not expect its SysBase
	   in a6.
	*/
	rts

#warning FIXME!!!
	/* get original SysBase */
	move.l	4.w,%a4
	/* call that Supervisor() function */
	jmp	Supervisor(%a4)

	/* Program flow will not get here */

 	/* a privileged do-nothing instruction */
 	or.w	#0x0000,%sr
 	/* No trap? Then this was called from supervisor mode. Prepare a rte. */
 	move.w	%sr,-(%sp)
 	jmp	(%a5)
 


 	/* CPU privilege violation vector points to here */
 	.globl	_TrapLevel8
_TrapLevel8:
 
 	/* There is only one legal location which may do a privilege
 	   violation - and that is the instruction above.
	*/
#warning What is this here???
/*
 	cmp.l	#_Exec_Supervisor,2.w(%sp)
*/
 	jne	pv
 	/* All OK. Prepare returnaddress and go to the right direction. */
 	move.l	#end,2.w(%sp)
 	jmp	(%a5)
end:	rts
 
 	/* Store trap number */
pv:	move.l	#8,-(%sp)
 	bra	_TrapEntry
 
 	/* And handle the trap */
_TrapEntry:
 	/* Simple disable */
 	or.w	#0x0700,%sr
 
 	/* get some room for destination address */
 	subq.w	#4,%sp
 
 	/* calculate destination address without clobbering any registers */
 	move.l	%a0,-(%sp)
	move.l	4,%a0
	bsr	get_offsetof_ThisTask
	move.l	(%a0,%d0),%a0
	bsr	get_offsetof_tc_TrapCode
	move.l	(%a0,%d0),4(%sp)
	move.l	(%sp)+,%a0

	/* and jump */
	rts
