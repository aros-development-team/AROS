/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME
	AROS_LH0(void, SetSR,

    LOCATION
	struct ExecBase *, SysBase, 24, Exec)

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
	.globl	AROS_SLIB_ENTRY(SetSR,Exec)
	.type	AROS_SLIB_ENTRY(SetSR,Exec),@function
AROS_SLIB_ENTRY(SetSR,Exec):
	/* Dummy */
	ret

