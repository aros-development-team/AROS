	TDNestCnt   =	303

	.text
	.align	16
	.globl	Exec_Forbid
	.type	Exec_Forbid,@function
Exec_Forbid:
	pushl %eax
	movl 8(%esp),%eax
	incb TDNestCnt(%eax)
	popl %eax
	ret
