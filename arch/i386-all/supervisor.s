/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME
	AROS_LH0(void, Supervisor,

    LOCATION
	struct ExecBase *, SysBase, 6, Exec)

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
	subl	$4,%esp
	pushl	%eax
	movl	12(%esp),%eax
	movl	%eax,4(%esp)
	popl	%eax
	ret
