#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:37  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#
#	__AROS_LH0(APTR, SuperState,
#
#   SYNOPSIS
#
#   LOCATION
#	struct ExecBase *, SysBase, 25, Exec)
#
#   FUNCTION
#	Enter supervisor mode (like Supervisor()), but return on the normal
#	user stack, so that user stack variables are still there. A call
#	to Userstate() will end this mode.
#
#   INPUTS
#
#   RESULT
#	Old supervisor stack. NULL if called from supervisor mode.
#
#   NOTES
#	The user stack must be big enough to hold all possible nestings
#	of interrupts.
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#	UserState(), Supervisor()
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	Supervisor  =	-0x1e

	.globl	_Exec_SuperState
_Exec_SuperState:
	| Goto supervisor mode. Preserve a5 in d0 (faster than stack space)
	movel	a5,d0
	leal	super,a5

	| Do not change user stack - use jmp
	jmp	a6@(Supervisor)
super:
	| Restore a5
	movel	d0,a5

	| Check if called from supervisor mode
	btst	#5,sp@
	jeq	fromuser

	| Called from supervisor mode. Just return NULL.
	moveql	#0,d0
	rte

fromuser:
	| Called from user mode. Restore sp and return supervisor sp.
	movel	sp,d0
	movel	usp,sp

	| usp already points to the returnaddress for the SuperState() call.
	rts

