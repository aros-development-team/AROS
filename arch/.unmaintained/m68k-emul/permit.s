/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec function Permit
    Lang: english
*/

/******************************************************************************

    NAME
        AROS_LH0(void, Permit,

    LOCATION
        struct ExecBase *, SysBase, 23, Exec)

    FUNCTION
	This function activates the dispatcher again after a call to Permit().

    INPUTS

    RESULT

    NOTES
	This function preserves all registers.
    EXAMPLE

    BUGS

    SEE ALSO
	Forbid(), Disable(), Enable()
	
    INTERNALS

    HISTORY

******************************************************************************/

	#include "machine.i"

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(Permit,Exec)
	.type	AROS_SLIB_ENTRY(Permit,Exec),@function
AROS_SLIB_ENTRY(Permit,Exec):
#if !UseRegisterArgs
	/* Preserve used registers */
	move.l	%a6,-(%sp)

	/* Get SysBase */
	move.l	8(%sp),%a6
#endif

	/* Decrement and test TDNestCnt */
	subq.b	#1,TDNestCnt(%a6)
	jbpl	.noswch

	/* return if there are no delayed switches pending. */
	btst	#7,AttnResched+1(%a6)
	jbeq	.noswch

	/* if IDNestCnt is not -1 taskswitches are still forbidden */
	tst.b	IDNestCnt(%a6)
	jbpl	.noswch

	/* Unset delayed switch bit and do the delayed switch */
	bclr	#7,AttnResched+1(%a6)
#if !UseRegisterArgs
	move.l	%a6,-(%sp)
#endif
	jsr	Switch(%a6)
#if !UseRegisterArgs
	addq	#4,%sp
#endif

	/* all done. */
.noswch:
#if !UseRegisterArgs
	move.l	(%sp)+,%a6
#endif
	rts

