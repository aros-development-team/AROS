#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.6  1996/11/01 02:05:25  aros
#    Motorola syntax (no more MIT)
#
#    Revision 1.5  1996/10/24 15:51:32  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.4  1996/10/24 01:38:31  aros
#    Include machine.i
#
#    Revision 1.3  1996/10/21 21:08:59  aros
#    Changed AROS_LA to AROS_LHA
#
#    Revision 1.2  1996/08/01 17:41:37  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#
#	AROS_LH1(void, StackSwap,
#
#   SYNOPSIS
#	AROS_LHA(struct StackSwapStruct *, newStack, A0),
#
#   LOCATION
#	struct ExecBase *, SysBase, 122, Exec)
#
#   FUNCTION
#	This function switches to the new stack given by the parameters in the
#	stackswapstruct structure. The old stack parameters are returned in
#	the same structure so that the stack can be restored later
#
#   INPUTS
#	newStack - parameters for the new stack
#
#   RESULT
#
#   NOTES
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_StackSwap
	.type	_Exec_StackSwap,@function

_Exec_StackSwap:
	| Preserve returnaddress and fix sp
	move.l	(sp)+,d0

	| Get pointer to tc_SPLower in a1 (tc_SPUpper is next)
	move.l	ThisTask(a6),a1
	lea.l	tc_SPLower(a1),a1

	| Just to be sure interrupts always find a good stackframe
	jsr	Disable(a6)

	| Swap Lower boundaries
	move.l	(a1),d1
	move.l	(a0),(a1)+
	move.l	d1,(a0)+

	| Swap higher boundaries
	move.l	(a1),d1
	move.l	(a0),(a1)
	move.l	d1,(a0)+

	| Swap stackpointers
	move.l	sp,d1
	move.l	(a0),sp
	move.l	d1,(a0)

	| Reenable interrupts.
	jsr	Enable(a6)

	| Restore returnaddress and return
	move.l	d1,-(sp)
	rts

