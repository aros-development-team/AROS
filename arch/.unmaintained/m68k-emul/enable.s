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
	/* Preserve all registers */
	move.l	%a6,-(%sp)
	
	/* Get SysBase */
	move.l	8(%sp),%a6
	
	/* Decrement and test IDNestCnt */
	subq.b	#1,IDNestCnt(%a6)
	jpl	.noswch
	bsr.w	AROS_CSYMNAME(en)

	/* return if there are no delayed switches pending. */
	tst.b	AttnResched+1(%a6)
	jpl	.noswch

	/* if TDNestCnt is not -1 taskswitches are still forbidden */
	tst.b	TDNestCnt(%a6)
	jpl	.noswch

	/* Unset delayed switch bit and do the delayed switch */
	bclr	#7,AttnResched+1(%a6)
	move.l	%a6,-(%sp)
	jsr	Switch(%a6)
	addq.w	#4,%sp

	/* all done. */
.noswch:
	move.l	(%sp)+,%a6
	rts

	.globl	AROS_CDEFNAME(en)
	.type	AROS_CDEFNAME(en),@function
AROS_CDEFNAME(en):
	movem.l	%d0-%d1,%a0-%a1,-(%sp)

	move.l	#-1,-(%sp)
	clr.l	-(%sp)
	pea	4(%sp)
	move.l	#1,-(%sp)
	jbsr	AROS_CSYMNAME(sigprocmask)
	addq.w	#8,%sp
	addq.w	#8,%sp

	movem.l	(%sp)+,%d0-%d1,%a0-%a1
	rts
