/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadGayle() - get the Gayle ID
    Lang: english
*/
/*****************************************************************************

    NAME
#include <proto/exec.h>

	AROS_LH0(ULONG, ReadGayle,

    LOCATION
	struct ExecBase *, SysBase, 136, Exec)

    FUNCTION
    Gets the Gayle ID

    INPUTS
	None.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(ReadGayle,Exec,136)
AROS_SLIB_ENTRY(ReadGayle,Exec,136):

	/* A3000 Fat Gary/RAMSEY registers are supposed to be supervisor-only
	 * Common chip revisions don't care but later (rare) revisions might do it */

	/* If A6 is 0, this is the initial ROM call, and
	 * we are already in supervisor mode.
	 */
	move.l	%a6,%d0
	beq.s	ss
	move.l	%a5,%sp@-
	lea		%pc@(sv),%a5
	jsr		Supervisor(%a6)
	move.l	%sp@+,%a5
	rts
sv:
	or.w	#0x0700,%sr
ss:
	move.l	#0xde1000,%a0       /* Gayle ID register */
	move.l	#0xdff000,%a1
	/* NOTE: We must check for custom chip mirroring. */
	/* Can we turn off interrupts? */
	move.w	0x1c(%a1),%sp@-		/* INTENAR */
	move.w	#0x7fff,0x9a(%a1)	/* INTENA */
	tst.w	0x1c(%a0)
	bne.s	0f					/* If non-zero, not INTENAR */
	move.w	#0xc000,0x9a(%a1)
	move.w	0x1c(%a0),%d0
	move.w	#0x7fff,0x9a(%a1)	/* INTENA */
	cmp.w	#0x4000,%d0
	beq.s	3f					/* If 0x4000, was INTENAR */

	/* Now, finally, we can check for a Gayle */
	clr.l	%d0                 /* ID we will return */
	moveq	#8-1,%d1            /* Gayle bit index */
	move.b	%d0,%a0@            /* Reset ID register. Gary based systems have
								 * timeout register in this address, it can be
								 * safely cleared */
0:	move.b	%a0@,%d0            /* Put reg into lower byte of %d0 */
	lsl.w	#1,%d0              /* Shift lower 16 bits of %d0 left by 1 */
	dbf		%d1,0b
	lsr.w	#8,%d0              /* Move ID to lower 8 bits of %d0 */

	cmp.b	#0xff,%d0
	bne.s	1f
3:
	clr.b	%d0
1:

	/* Restore interrupts. */
	move.w	%sp@+,%d1
	or.w	#0x8000,%d1
	move.w	%d1,0x9a(%a1)		/* INTENA */

	move.l	%a6,%d1
	beq.s	2f
	rte
2:	rts
