#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.7  1996/10/24 15:51:11  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.6  1996/10/23 08:04:25  aros
#    Use generated offsets which makes porting much easier
#
#    Revision 1.5  1996/09/11 16:54:27	digulla
#    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
#	some systems name an external symbol "x" as "_x" and others as "x".
#	(The problem arises with assembler symbols which might differ)
#
#    Revision 1.4  1996/08/23 16:49:21	digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#	I replaced it with .balign which does what we want.
#
#    Revision 1.3  1996/08/13 14:03:19	digulla
#    Added standard headers
#
#    Revision 1.2  1996/08/01 17:41:10	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#	AROS_LH0(void, Exception,
#
#   LOCATION
#	struct ExecBase *, SysBase, 8, Exec)
#
#   FUNCTION
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_Exception
	.type	_Exec_Exception,@function
_Exec_Exception:
	/* Get SysBase amd pointer to current task */
	movl	4(%esp),%ebp
	movl	ThisTask(%ebp),%edi

	/* Clear exception flag */
	andb	$~TF_EXCEPT,tc_Flags(%edi)

	/* If the exception is raised out of Wait IDNestCnt may be >0 */
	movb	IDNestCnt(%ebp),%ebx
	/* Set it to a defined value */
	movb	$0,IDNestCnt(%ebp)

exloop: /* Get mask of signals causing the exception */
	movl	tc_SigExcept(%edi),%ecx
	andl	tc_SigRecvd(%edi),%ecx
	je	excend

	/* Clear bits */
	xorl	%ecx,tc_SigExcept(%edi)
	xorl	%ecx,tc_SigRecvd(%edi)

	/* Raise exception. Enable Interrupts */
	movl	tc_ExceptData(%edi),%eax
	leal	Enable(%ebp),%edx
	pushl	%ebp
	call	*%edx
	pushl	%ebp
	pushl	%eax
	pushl	%ecx
	movl	tc_ExceptCode(%edi),%edx
	call	*%edx
	leal	Disable(%ebp),%edx
	pushl	%ebp
	call	*%edx
	addl	$20,%esp

	/* Re-use returned bits */
	orl	%eax,tc_SigExcept(%edi)
	jmp	exloop

excend: /* Restore IDNestCnt and return */
	movb	%ebx,IDNestCnt(%ebp)
	ret
