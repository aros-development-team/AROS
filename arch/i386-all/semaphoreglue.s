	/*    The following functions are guaranteed to preserve	*/
	/*    all registers. But I don't want to write them completely	*/
	/*    in assembly - C is generally more readable.		*/
	/*    So I use those stubs to preserve the registers.		*/

	.text
	.align	16
	.globl	Exec__ObtainSemaphore
	.type	Exec__Obtainsemaphore,@function
Exec__ObtainSemaphore:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	Exec_ObtainSemaphore
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	Exec__ReleaseSemaphore
	.type	Exec__ReleaseSemaphore,@function
Exec__ReleaseSemaphore:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	Exec_ReleaseSemaphore
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	Exec__ObtainSemaphoreShared
	.type	Exec__ObtainSemaphoreShared,@function
Exec__ObtainSemaphoreShared:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	Exec_ObtainSemaphoreShared
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

