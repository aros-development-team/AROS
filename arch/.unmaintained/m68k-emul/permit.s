/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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
	/* Preserve used registers */
	move.l	%a1,-(%sp)

	/* Get SysBase */
	move.l	8(%sp),%a1

	/* Decrement and test TDNestCnt */
	subq.b	#1,TDNestCnt(%a1)
	jpl	.noswch

	/* return if there are no delayed switches pending. */
	tst.b	AttnResched+1(%a1)
	jpl	.noswch

	/* if IDNestCnt is not -1 taskswitches are still forbidden */
	tst.b	IDNestCnt(%a1)
	jpl	.noswch

	/* Unset delayed switch bit and do the delayed switch */
	bclr	#7,AttnResched+1(%a1)
	move.l	%a1,-(%sp)
	jsr	Switch(%a1)
	addq	#4,%sp
	
	/* all done. */
.noswch:
	move.l	(%sp)+,%a1
	rts

