/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Permit() - Allow tasks switches to occur.
    Lang: english
*/
/*  NAME

        AROS_LH0(void, Permit,

    LOCATION
        struct ExecBase *, SysBase, 23, Exec)

    FUNCTION
        This function will reactivate the task dispatcher after a call
        to Forbid(). Note that calls to Forbid() nest, and for every
        call to Forbid() you need a matching call to Permit().

    INPUTS
        None.

    RESULT
        Multitasking will be re-enabled.

    NOTES
        This function preserves all registers.

        To prevent deadlocks calling Wait() in forbidden state breaks
        the forbid - thus taskswitches may happen again.

    EXAMPLE
        No you really don't want to use this function.

    SEE ALSO
        Forbid(), Disable(), Enable(), Wait()

HISTORY

******************************************************************************/
	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Permit,Exec)

AROS_SLIB_ENTRY(Permit,Exec):
	/* As we said above: ALL registers must be preserved! */
	subq.b	#1,%a6@(TDNestCnt)	/* Drop the TD Nesting */
	bge.s	0f			/* If >= 0, we're done */
	tst.b	%a6@(IDNestCnt)		/* If >= 0, we're done */
	bge.s	0f
	
	/* Do we need to switch tasks? The AttnResched
	 * field will tell us!
	 */
	btst	#7,%a6@(AttnResched)	/* 7 is ARF_AttnSwitch */
	beq.s	0f			/* If == 0, don't reschedule */

	/* Call Supervisor mode, to let us know what mode
	 * we're in.
	 */
	move.l	%a5,%sp@-		/* Save A5 for later */
	lea	1f,%a5			/* Set A5 to address of our function */
	jsr	%a6@(Supervisor)	/* Do it in Exec/Supervisor mode! */
	move.l	%sp@+,%a5		/* Restore A5 */
0:	rts

1:
	btst	#(13 - 8),%sp@		/* Bottom byte on the stack is SR mode */
	beq.s	2f
	rte				/* We were in Supervisor mode already !
					 * No task switching for us! */
2:	/* Switch to the next task */
	jsr	%a6@(Switch)
	rte				/* Back to the original caller */
