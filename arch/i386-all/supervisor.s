	.text
	.align	16
	.globl	Exec_Supervisor
	.type	Exec_Supervisor,@function
Exec_Supervisor:
	/* The emulation has no real supervisor mode. */
	subl	$4,%esp
	pushl	%eax
	movl	12(%esp),%eax
	movl	%eax,4(%esp)
	popl	%eax
	ret
