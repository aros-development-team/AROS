/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME
	AROS_LH0(void, Permit,

    LOCATION
	struct ExecBase *, SysBase, 23, Exec)

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
	.globl	AROS_SLIB_ENTRY(Permit,Exec)
	.type	AROS_SLIB_ENTRY(Permit,Exec),@function
AROS_SLIB_ENTRY(Permit,Exec):
	/* Preserve all registers */
	pushl	%edx
	pushl	%eax

	/* Get SysBase */
	movl	12(%esp),%edx

	/* Decrement and test TDNestCnt */
	decb	TDNestCnt(%edx)
	jge	noswch

	/* Is there a delayed switch pending and are taskswitches allowed? */
	cmpb	$0,AttnResched(%edx)
	jge	noswch
	cmpb	$0,IDNestCnt(%edx)
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
