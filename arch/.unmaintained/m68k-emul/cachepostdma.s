/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Exec function CachePostDMA
    Lang: english
*/

/******************************************************************************

    NAME
	AROS_LH0(void, CachePostDMA,

    LOCATION
	struct ExecBase *, SysBase, 128, Exec)

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
	.globl	AROS_SLIB_ENTRY(CachePostDMA,Exec)
	.type	AROS_SLIB_ENTRY(CachePostDMA,Exec),@function
AROS_SLIB_ENTRY(CachePostDMA,Exec):
	/* Dummy */
	rts

