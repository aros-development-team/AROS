/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec function GetCC
    Lang: english
*/

/******************************************************************************

    NAME
	AROS_LH0(void, GetCC,

    LOCATION
	struct ExecBase *, SysBase, 88, Exec)

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
	.globl	AROS_SLIB_ENTRY(GetCC,Exec)
	.type	AROS_SLIB_ENTRY(GetCC,Exec),@function
AROS_SLIB_ENTRY(GetCC,Exec):
	/* This should be implemented in the jump table for speed. */
	move	%ccr,%d0
	rts

