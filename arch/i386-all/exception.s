/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME
	AROS_LH0(void, Exception,

    LOCATION
	struct ExecBase *, SysBase, 8, Exec)

    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

	#include "machine.i"

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(Exception,Exec)
	.type	AROS_SLIB_ENTRY(Exception,Exec),@function
AROS_SLIB_ENTRY(Exception,Exec):
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
