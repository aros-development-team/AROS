	Exception   =	-40
	Disable     =	-100
	Enable	    =	-105
	ThisTask    =	284
	IDNestCnt   =	302
	TaskReady   =	428
	tc_Flags    =	16
	tc_State    =	17
	tc_IDNestCnt=	18
	tc_SPReg    =	56
	tc_Switch   =	68
	tc_Launch   =	72
	TF_EXCEPT   =	32
	TF_SWITCH   =	64
	TS_RUN      =	2

	.text
	.align	16
	.globl	Exec_Dispatch
	.type	Exec_Dispatch,@function
Exec_Dispatch:
	/* Push all registers */
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi
	pushl	%esi
	pushl	%ebp

	/* Get SysBase */
	movl	32(%esp),%ecx

	/* Store sp */
	movl	ThisTask(%ecx),%edx
	movl	%esp,tc_SPReg(%edx)

	/* Switch bit set? */
	testb	$TF_SWITCH,tc_Flags(%edx)
	je	noswch
	movl	tc_Switch(%edx),%eax
	call	*%eax

	/* Store IDNestCnt */
noswch:	movb	IDNestCnt(%ecx),%al
	movb	%al,tc_IDNestCnt(%edx)
	movb	$-1,IDNestCnt(%ecx)

	/* Get task from ready list */
	movl	TaskReady(%ecx),%edx
	movl	(%edx),%eax
	movl	%eax,TaskReady(%ecx)
	movl	(%edx),%eax
	leal	TaskReady(%ecx),%ebx
	movl	%ebx,4(%eax)
	movl	%edx,ThisTask(%ecx)

	/* Use as current task */
	movb	$TS_RUN,tc_State(%edx)
	movb	tc_IDNestCnt(%edx),%al
	movb	%al,IDNestCnt(%ecx)

	/* Launch bit set? */
	cmpb	$0,tc_Flags(%edx)
	jge	nolnch
	movl	tc_Launch(%edx),%eax
	call	*%eax

	/* Get new sp */
nolnch:	movl	tc_SPReg(%edx),%esp

	/* Except bit set? */
	testb	$TF_EXCEPT,tc_Flags(%edx)
	je	noexpt

	/* Raise task exception in Disable()d state */
	pushl	%ecx
	leal	Disable(%ecx),%eax
	call	*%eax
	leal	Exception(%ecx),%eax
	call	*%eax
	movl	(%esp),%ecx
	leal	Enable(%ecx),%eax
	call	*%eax
	addl	$4,%esp

	/* Restore registers and return */
noexpt:	popl	%ebp
	popl	%esi
	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	ret
