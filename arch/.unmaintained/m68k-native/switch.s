|*****************************************************************************
|
|   NAME
|
|	__AROS_LH0(void, Switch,
|
|   LOCATION
|	struct ExecBase *, SysBase, 6, Exec)
|
|   FUNCTION
|	Tries to switch to the first task in the ready list. This
|	function works almost like Dispatch() with the slight difference
|	that it may be called at any time and as often as you want and
|	that it doesn't lose the current task if it is of type TS_RUN.
|
|   INPUTS
|
|   RESULT
|
|   NOTES
|	This function is CPU dependant.
|
|	This function is for internal use by exec only.
|
|	This function preserves all registers.
|
|   EXAMPLE
|
|   BUGS
|
|   SEE ALSO
|	Dispatch()
|
|   INTERNALS
|
|   HISTORY
|
|******************************************************************************

	Supervisor  =	-0x1e
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
	| call switch in supervisor mode
	| this is necessary to determine if the current context is user or
	| supervisor mode
	movel	a5,sp@-
	movel	#switch,a5
	jsr	a6@(Supervisor)
	movel	sp@+,a5
	rts

switch:
	| test if called from supervisor mode
	| (supervisor bit is bit 8+5 of sr when calling Switch() )
	btst	#5,sp@
	jeq	nosup

	| called from supervisor mode (grrrr)
	| since I can only Dispatch() when falling down to user mode I
	| must do it later - set the delayed dispatch flag and return
	bset	#7,a6@(AttnResched)
end:	rte

	| Called from user mode
	| Always disable interrupts when testing task lists
nosup:	movew	#0x2700,sr

	| Preserve scratch registers
	moveml	d0/d1/a0/a1,sp@-

	| If not in state TS_RUN the current task is part of one of the
	| task lists.
	movel	a6@(ThisTask),a1
	cmpb	#TS_RUN,a1@(tc_State)
	jne	disp

	| If TB_EXCEPT is not set...
	btst	#TB_EXCEPT,a1@(tc_Flags)
	jne	disp

	| ...Move task to the ready list
	moveb	#TS_READY,a1@(tc_State)
	leal	a6@(TaskReady),a0
	jsr	a6@(Enqueue)

	| dispatch
disp:	moveml	sp@+,d0/d1/a0/a1
	jmp	a6@(Dispatch)
