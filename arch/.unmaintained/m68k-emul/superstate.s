#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.1  1996/12/05 15:31:01  aros
#    Patches by Geert Uytterhoeven integrated
#
#    Revision 1.4  1996/11/01 02:05:25  aros
#    Motorola syntax (no more MIT)
#
#    Revision 1.3  1996/10/24 15:51:32  aros
#    Use the official AROS macros over the __AROS versions.
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
#	AROS_LH0(APTR, SuperState,
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

	.include "machine.i"

	.globl	_Exec_SuperState
	.type	_Exec_SuperState,@function
_Exec_SuperState:
	| Dummy
	rts

