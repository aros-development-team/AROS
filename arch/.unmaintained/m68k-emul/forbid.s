/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Exec function Forbid
    Lang: english
*/

/******************************************************************************

    NAME
        AROS_LH0(void, Forbid,

    LOCATION
        struct ExecBase *, SysBase, 22, Exec)

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
	.globl	AROS_SLIB_ENTRY(Forbid,Exec)
	.type	AROS_SLIB_ENTRY(Forbid,Exec),@function
AROS_SLIB_ENTRY(Forbid,Exec):
	linkw	%fp,#0
	move.l	8(%fp),%a1
	addq.b	#1,TDNestCnt(%a1)
	unlk	%fp
	rts

