#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.5  1996/11/01 02:05:25  aros
#    Motorola syntax (no more MIT)
#
#    Revision 1.4  1996/10/24 15:51:33  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.3  1996/10/24 01:38:31  aros
#    Include machine.i
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
#	AROS_LH0(void, Switch,
#
#   LOCATION
#	struct ExecBase *, SysBase, 6, Exec)
#
#   FUNCTION
#	Tries to switch to the first task in the ready list. This
#	function works almost like Dispatch() with the slight difference
#	that it may be called at any time and as often as you want and
#	that it doesn't lose the current task if it is of type TS_RUN.
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#	This function is CPU dependant.
#
#	This function is for internal use by exec only.
#
#	This function preserves all registers.
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#	Dispatch()
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_Switch
	.type	_Exec_Switch,@function

_Exec_Switch:
	| call switch in supervisor mode
	| this is necessary to determine if the current context is user or
	| supervisor mode
	movel	a5,-(sp)
	movel	#switch,a5
	jsr	Supervisor(a6)
	movel	(sp)+,a5
	rts

switch:
	| test if called from supervisor mode
	| (supervisor bit is bit 8+5 of sr when calling Switch() )
	btst	#5,(sp)
	jeq	nosup

	| called from supervisor mode (grrrr)
	| since I can only Dispatch() when falling down to user mode I
	| must do it later - set the delayed dispatch flag and return
	bset	#7,AttnResched(a6)
end:	rte

	| Called from user mode
	| Always disable interrupts when testing task lists
nosup:	movew	#0x2700,sr

	| Preserve scratch registers
	moveml	d0/d1/a0/a1,-(sp)

	| If not in state TS_RUN the current task is part of one of the
	| task lists.
	movel	ThisTask(a6),a1
	cmpb	#TS_RUN,tc_State(a1)
	jne	disp

	| If TB_EXCEPT is not set...
	btst	#TB_EXCEPT,tc_Flags(a1)
	jne	disp

	| ...Move task to the ready list
	moveb	#TS_READY,tc_State(a1)
	leal	TaskReady(a6),a0
	jsr	Enqueue(a6)

	| dispatch
disp:	moveml	(sp)+,d0/d1/a0/a1
	jmp	Dispatch(a6)
