/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME
	AROS_LH0(void, Enable,

    LOCATION
	struct ExecBase *, SysBase, 21, Exec)

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
	.globl	AROS_SLIB_ENTRY(Enable,Exec)
	.type	AROS_SLIB_ENTRY(Enable,Exec),@function
AROS_SLIB_ENTRY(Enable,Exec):
	/* Preserve all registers */
	pushl	%edx
	pushl	%eax

	/* Get SysBase */
	movl	12(%esp),%edx

	/* Decrement and test IDNestCnt */
	decb	IDNestCnt(%edx)
	jge	noswch
	call	en

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
noswch: popl	%eax
	popl	%edx
	ret

.globl en
	.type	en,@function
en:
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl $-1
	pushl $0
	leal 4(%esp),%eax
	pushl %eax
	pushl $1
	call AROS_CSYMNAME(sigprocmask)
	addl $16,%esp
	popl %edx
	popl %ecx
	popl %eax
	ret
