	IDNestCnt   =	302

	.text
	.align	16
	.globl	Exec_Disable
	.type	Exec_Disable,@function
Exec_Disable:
	pushl %eax
	movl 8(%esp),%eax
	incb IDNestCnt(%eax)
	popl %eax
	ret
