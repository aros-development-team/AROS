#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:32  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	Dispatch    =	-0x2a
	Enqueue     =	-0x10e
	ThisTask    =	0x114
	AttnResched =	0x12a
	TaskReady   =	0x196
	tc_Flags    =	0xe
	tc_State    =	0xf
	TS_RUN	    =	2
	TS_READY    =	3
	TB_EXCEPT   =	5

	.globl	_Exec_Switch
_Exec_Switch:
	# Preserve scratch registers
	moveml	d0/d1/a0/a1,sp@-

	# If not in state TS_RUN the current task is already moved
	# to one of the task lists.
	movel	a6@(ThisTask),a1
	cmpb	#TS_RUN,a1@(tc_State)
	jne	disp

	# If TB_EXCEPT is not set...
	btst	#TB_EXCEPT,a1@(tc_Flags)
	jne	disp

	# ...move task to the ready list
	moveb	#TS_READY,a1@(tc_State)
	leal	a6@(TaskReady),a0
	jsr	a6@(Enqueue)

	# dispatch
disp:	moveml	sp@+,d0/d1/a0/a1
	jmp	a6@(Dispatch)
