/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME
	AROS_LH1(void, Supervisor,

    SYNOPSIS
	AROS_LHA(ULONG_FUNC, userFunction, A5),

    LOCATION
	struct ExecBase *, SysBase, 5, Exec)

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
	.globl	AROS_SLIB_ENTRY(Supervisor,Exec)
	.type	AROS_SLIB_ENTRY(Supervisor,Exec),@function
AROS_SLIB_ENTRY(Supervisor,Exec):
	/* The emulation has no real supervisor mode. */
	subl	$4,%esp 	/* Delete return addess */
	pushl	%eax		/* Save a register */
	movl	12(%esp),%eax   /* Get function pointer */
	movl	%eax,4(%esp)    /* Put function pointer on the stack */
	popl	%eax		/* Restore register */
	ret			/* Call user function */
