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
	bsr.w	AROS_CSYMNAME(disable)
	move.l	%a6,-(%sp)
	
	/* Get SysBase */
	move.l	8(%sp),%a6
	
	/* increment nesting count and return */
	addq.b	#1,IDNestCnt(%a6)
	move.l	(%sp)+,%a6
	rts

	.globl	AROS_CDEFNAME(disable)
	.type	AROS_CDEFNAME(disable),@function
AROS_CDEFNAME(disable):
	movem.l	%d0-%d1/%a0-%a1,-(%sp)

	move.l	#-1,-(%sp)
	clr.l	-(%sp)
	pea	4(%sp)
	move.l	#SIG_BLOCK,-(%sp)
	jbsr	AROS_CSYMNAME(sigprocmask)
	addq.w	#8,%sp
	addq.w	#8,%sp

	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

