/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#if !UseRegisterArgs
	/* Preserve all registers */
	move.l	%a6,-(%sp)
	
	/* Get SysBase */
	move.l	8(%sp),%a6
#endif

	/* Decrement and test IDNestCnt */
	subq.b	#1,IDNestCnt(%a6)
	jbpl	.noswch
	jbsr	AROS_CSYMNAME(os_enable)

	/* return if there are no delayed switches pending. */
	btst	#7,AttnResched+1(%a6)
	jbeq	.noswch

	/* if TDNestCnt is not -1 taskswitches are still forbidden */
	tst.b	TDNestCnt(%a6)
	jbpl	.noswch

	/* Unset delayed switch bit and do the delayed switch */
	bclr	#7,AttnResched+1(%a6)
#if !UseRegisterArgs
	move.l	%a6,-(%sp)
#endif
	jsr	Switch(%a6)
#if !UseRegisterArgs
	addq.w	#4,%sp
#endif

	/* all done. */
.noswch:
#if !UseRegisterArgs
	move.l	(%sp)+,%a6
#endif
	rts
