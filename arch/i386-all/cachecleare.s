/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME
	AROS_LH3(void, CacheClearE,

    SYNOPSIS
	AROS_LHA(APTR,  address, A0),
	AROS_LHA(ULONG, length,  D0),
	AROS_LHA(ULONG, caches,  D1),

    LOCATION
	struct ExecBase *, SysBase, 107, Exec)

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
	.globl	AROS_SLIB_ENTRY(CacheClearE,Exec)
	.type	AROS_SLIB_ENTRY(CacheClearE,Exec),@function
AROS_SLIB_ENTRY(CacheClearE,Exec):
	/* Dummy */
	ret

