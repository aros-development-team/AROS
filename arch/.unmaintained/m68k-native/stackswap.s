#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
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
	movel	sp@+,d0

	| Get pointer to tc_SPLower in a1 (tc_SPUpper is next)
	movel	a6@(ThisTask),a1
	leal	a1@(tc_SPLower),a1

	| Just to be sure interrupts always find a good stackframe
	jsr	a6@(Disable)

	| Swap Lower boundaries
	movel	a1@,d1
	movel	a0@,a1@+
	movel	d1,a0@+

	| Swap higher boundaries
	movel	a1@,d1
	movel	a0@,a1@
	movel	d1,a0@+

	| Swap stackpointers
	movel	sp,d1
	movel	a0@,sp
	movel	d1,a0@

	| Reenable interrupts.
	jsr	a6@(Enable)

	| Restore returnaddress and return
	movel	d1,sp@-
	rts

