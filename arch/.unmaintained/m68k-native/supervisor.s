#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.6  1996/11/16 01:32:00  aros
#    Cheat and use the Amiga's real Supervisor() routine (for now)
#
#    Revision 1.5  1996/11/01 02:05:25  aros
#    Motorola syntax (no more MIT)
#
#    Revision 1.4  1996/10/24 15:51:32  aros
#    Use the official AROS macros over the __AROS versions.
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
#	AROS_LH1(ULONG, Supervisor,
#
#   SYNOPSIS
#	AROS_LHA(ULONG_FUNC, userFunction, A5),
#
#   LOCATION
#	struct ExecBase *, SysBase, 5, Exec)
#
#   FUNCTION
#	Call a routine in supervisor mode. This routine runs on the
#	supervisor stack and must end with a 'rte'. No registers are spilled,
#	i.e. Supervisor() effectively works like a function call.
#
#   INPUTS
#	userFunction - address of the function to be called.
#
#   RESULT
#	whatever the function left in the registers
#
#   NOTES
#	This function is CPU dependant.
#
#   EXAMPLE
#
#   BUGS
#	Context switches that happen during the duration of this call are lost.
#
#   SEE ALSO
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	.include "machine.i"

	tc_TrapCode =	0x32

	.text
	.balign 16
	.globl	_Exec_Supervisor
	.type   _Exec_Supervisor,@function
_Exec_Supervisor:
	| I'm cheating my way through this routine, because ixemul traps all
	| exceptions. We would never be able to finish this function.
	| I need to change one register, though, because I need to get this
	| Amiga's original ExecBase. I'll use register a4 for this. I also
	| assume the original Supervisor() function doesn't expect it's SysBase
	| in a6.

	move.l	4.w,a4			| get original SysBase
	jmp	Supervisor(a4)		| call that Supervisor() function

|	| a privileged do-nothing instruction
|	or.w	#0x0000,sr
|	| No trap? Then this was called from supervisor mode. Prepare a rte.
|	move.w	sr,-(sp)
|	jmp	(a5)
|
|	| CPU privilege violation vector points to here
|	.globl	_TrapLevel8
|_TrapLevel8:
|
|	| There's only one legal location which may do a privilege
|	| violation - and that's the instruction above.
|	cmp.l	#_Exec_Supervisor,2.w(sp)
|	jne	pv
|	| All OK. Prepare returnaddress and go to the right direction.
|	move.l	#end,2.w(sp)
|	jmp	(a5)
|end:	rts
|
|	| Store trap number
|pv:	move.l	#8,-(sp)
|	bra	_TrapEntry
|
|	| And handle the trap
|_TrapEntry:
|	| Simple disable
|	or.w	#0x0700,sr
|
|	| get some room for destination address
|	subq.w	#4,sp
|
|	| calculate destination address without clobbering any registers
|	move.l	a0,-(sp)
|	move.l	4,a0
|	move.l	ThisTask(a0),a0
|	move.l	tc_TrapCode(a0),4(sp)
|	move.l	(sp)+,a0
|
|	| and jump
|	rts
