	IDNestCnt   =	302
	AttnResched =	306
	TDNestCnt   =	303
	Switch      =	-30

	.text
	.align	16
	.globl	Exec_Enable
	.type	Exec_Enable,@function
Exec_Enable:
	/* Preserve all registers */
	pushl	%edx
	pushl	%eax

	/* Get SysBase */
	movl	12(%esp),%edx

	/* Decrement and test IDNestCnt */
	decb	IDNestCnt(%edx)
	jge	noswch

	/* Is there a delayed switch pending and are taskswitches allowed? */
	cmpb	$0,AttnResched(%edx)
	jge	noswch
	cmpb	$0,TDNestCnt(%edx)
	jge	noswch

	/* Clear delayed switch bit and do the delayed switch */
	andb	$127,AttnResched(%edx)
	leal	Switch(%edx),%eax
	pushl	%edx
	call	*%eax
	addl	$4,%esp

	/* Restore registers and return */
noswch:	popl	%eax
	popl	%edx
	ret
