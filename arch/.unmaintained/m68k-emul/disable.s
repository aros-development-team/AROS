/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Exec function Disable
    Lang: english
*/

/******************************************************************************

    NAME
        AROS_LH0(void, Disable,

    LOCATION
        struct ExecBase *, SysBase, 20, Exec)

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
	.globl	AROS_SLIB_ENTRY(Disable,Exec)
	.type	AROS_SLIB_ENTRY(Disable,Exec),@function

AROS_SLIB_ENTRY(Disable,Exec):
	bsr.w	AROS_CDEFNAME(disable)
	linkw	%fp,#0
	move.l	%a2,-(%sp)
	/* Get SysBase */
	move.l	8(%fp),%a2
	/* increment nesting count and return */
	addq.b	#1,IDNestCnt(%a2)
	move.l	-4(%fp),%a2
	unlk	%fp
	rts

	.globl	AROS_CDEFNAME(disable)
	.type	AROS_CDEFNAME(disable),@function
AROS_CDEFNAME(disable):
	linkw	%fp,#0
	move.l	#-1,-(%sp)
	clr.l	-(%sp)
	pea	4(%sp)
	clr.l	-(%sp)
	jbsr	AROS_CSYMNAME(sigprocmask)
	unlk	%fp
	rts

