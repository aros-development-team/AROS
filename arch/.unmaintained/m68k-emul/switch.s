#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/12/05 15:31:02  aros
#    Patches by Geert Uytterhoeven integrated
#
#    Revision 1.2  1996/08/01 17:41:32  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	.include "machine.i"

	.globl	_Exec_Switch
	.type	_Exec_Switch,@function
_Exec_Switch:
	# Preserve scratch registers
	moveml	%d0/%d1/%a0/%a1,%sp@-

	# If not in state TS_RUN the current task is already moved
	# to one of the task lists.
	movel	%a6@(ThisTask),%a1
	cmpb	#TS_RUN,%a1@(tc_State)
	jne	disp

	# If TB_EXCEPT is not set...
	btst	#TB_EXCEPT,%a1@(tc_Flags)
	jne	disp

	# ...move task to the ready list
	moveb	#TS_READY,%a1@(tc_State)
	leal	%a6@(TaskReady),%a0
	jsr	%a6@(Enqueue)

	# dispatch
disp:	moveml	%sp@+,%d0/%d1/%a0/%a1
	jmp	%a6@(Dispatch)
