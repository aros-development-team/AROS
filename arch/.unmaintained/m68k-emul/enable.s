/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Exec function Enable
    Lang: english
*/

/******************************************************************************

    NAME
        AROS_LH0(void, Enable,

    LOCATION
        struct ExecBase *, SysBase, 21, Exec)

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
	.globl	AROS_SLIB_ENTRY(Enable,Exec)
	.type	AROS_SLIB_ENTRY(Enable,Exec),@function
AROS_SLIB_ENTRY(Enable,Exec):
	linkw	%fp,#0
	/* Get SysBase */
	move.l	8(%fp),%a0
	/* Decrement and test IDNestCnt */
	subq.b	#1,IDNestCnt(%a0)
	jpl	.noswch
	bsr.w	en

	/* return if there are no delayed switches pending. */
	tst.b	AttnResched+1(%a0)
	jpl	.noswch

	/* if TDNestCnt is not -1 taskswitches are still forbidden */
	tst.b	TDNestCnt(%a0)
	jpl	.noswch

	/* Unset delayed switch bit and do the delayed switch */
	bclr	#7,AttnResched+1(%a0)
	move.l	%a0,-(%sp)
	jsr	Switch(%a0)
	addq.w	#4,%sp

	/* all done. */
.noswch:
	unlk 	%fp
	rts

	.globl	en
	.type	en,@function
en:
	linkw	%fp,#0
	move.l	#-1,-(%sp)
	clr.l	-(%sp)
	pea	4(%sp)
	move.l	#1,-(%sp)
	jbsr	AROS_CSYMNAME(sigprocmask)
	unlk	%fp
	rts
