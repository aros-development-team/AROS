	Exception   =	-0x30
	Disable     =	-0x78
	Enable	    =	-0x7e
	ThisTask    =	0x114
	IDNestCnt   =	0x126
	TaskReady   =	0x196
	tc_Flags    =	0xe
	tc_State    =	0xf
	tc_IDNestCnt=	0x10
	tc_SPReg    =	0x36
	tc_Switch   =	0x42
	tc_Launch   =	0x46
	TS_RUN	    =	2
	TB_EXCEPT   =	5
	TB_SWITCH   =	6
	TB_LAUNCH   =	7

	| Dispatching routine for the 68000.
	| Higher models (with FPU) need a slightly different
	| routine or the additional registers cannot be used!

	.globl	_Exec_Dispatch
_Exec_Dispatch:

	| move whole user context to user stack
	moveml	d0-d7/a0-a6,sp@-

	| get current task and store sp there
	movel	a6@(ThisTask),a2
	movel	sp,a2@(tc_SPReg)

	| call the switch routine if necessary
	btst	#TB_SWITCH,a2@(tc_Flags)
	jeq	noswch
	movel	a2@(tc_Switch),a5
	jsr	a5@

	| store IDNestCnt
noswch: moveb	a6@(IDNestCnt),a2@(tc_IDNestCnt)
	moveb	#-1,a6@(IDNestCnt)

	| get address of ready list
	leal	a6@(TaskReady),a0

	| there must be a ready task
	movel	a0@,a2
	movel	a2@,a1

	| remove first ready task in the list
	movel	a1,a0@
	movel	a0,a1@(4:W)

	| and use it as new current task
	movel	a2,a6@(ThisTask)
	moveb	#TS_RUN,d0
	moveb	d0,a2@(tc_State)

	| restore IDNestCnt
	moveb	a2@(tc_IDNestCnt),a6@(IDNestCnt)

	| call the launch routine if necessary
	btst	#TB_LAUNCH,a2@(tc_Flags)
	jeq	nolnch
	movel	a2@(tc_Launch),a5
	jsr	a5@

	| get stack pointer
nolnch: movel	a2@(tc_SPReg),sp

	| test task exception bit
	btst	#TB_EXCEPT,a2@(tc_Flags)
	jeq	noexc

	| Raise a task exception in Disable()d state.
	jsr	a6@(Disable)
	jsr	a6@(Exception)
	jsr	a6@(Enable)

	| restore context
noexc:	moveml	sp@+,d0-d7/a0-a6
	rts

