/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec function Supervisor
    Lang: english
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
	jmp (%a5)

