#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.4  1996/09/11 16:54:28  digulla
#    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
#    	some systems name an external symbol "x" as "_x" and others as "x".
#    	(The problem arises with assembler symbols which might differ)
#
#    Revision 1.3  1996/08/23 16:49:21	digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#	I replaced it with .balign which does what we want.
#
#    Revision 1.2  1996/08/01 17:41:19	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	/*    The following functions are guaranteed to preserve	*/
	/*    all registers. But I don't want to write them completely  */
	/*    in assembly - C is generally more readable.		*/
	/*    So I use those stubs to preserve the registers.		*/

	.text
	.balign 16
	.globl	_Exec__ObtainSemaphore
	.type	_Exec__Obtainsemaphore,@function
_Exec__ObtainSemaphore:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	_Exec_ObtainSemaphore
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	_Exec__ReleaseSemaphore
	.type	_Exec__ReleaseSemaphore,@function
_Exec__ReleaseSemaphore:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	_Exec_ReleaseSemaphore
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	_Exec__ObtainSemaphoreShared
	.type	_Exec__ObtainSemaphoreShared,@function
_Exec__ObtainSemaphoreShared:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	_Exec_ObtainSemaphoreShared
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

