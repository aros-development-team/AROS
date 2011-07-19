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
	cmp.l	#0,%a6
	beq.s   2f
	move.l	%a5,%sp@-
	lea		%pc@(2f),%a5
	jsr		Supervisor(%a6)
	move.l	%sp@+,%a5
	rts
	
2:
	/* NOTE: We must check for custom chip mirroring.
	 */
	/* Can we turn off interrupts? */
	move.w	0xdff01c,%d0                    /* INTENAR */
	move.l	#(0xde1000 & ~0x3ffff), %a0	/* Chip mirror base */
	move.w	#0x7fff,%a0@(-(0x40000 - 0x3f09a))
	tst.w	%a0@(-(0x40000 - 0x3f01c))	/* If non-zero, this is not INTENAR */
	bne.s	0f
	/* It was zero ... */
	move.w	#0xc000,%a0@(-(0x40000 - 0x3f09a))    /* Try the master enable */
	tst.w	%a0@(-(0x40000 - 0x3f01c))           /* If still zero, not INTENAR */
	beq.s	0f
	/* Restore interrupts. */
	move.w	0x7fff,0xdff09a         /* INTENA */
	or.w	#0x8000,%d0
	move.w	%d0,0xdff09a            /* INTENA */
	/* It was a custom chip mirror. Can't be Gayle*/
	clr.l	%d0
	bra.s	1f
0:
	move.l  #0xde1000,%a0       /* Gayle ID register */

	tst.b	%a0@(-0x1000)		/* clear possible Fat Gary Timeout register
								 * to prevent bogus 0x80 Gayle ID on an A3000 */

    /* Now, finally, we can check for a Gayle */
	clr.l   %d0                 /* ID we will return */
	moveq	#8-1,%d1            /* Gayle bit index */
	move.b  #0,%a0@             /* Reset ID register */
0:	move.b  %a0@,%d0            /* Put reg into lower byte of %d0 */
	lsl.w   #1,%d0              /* Shift lower 16 bits of %d0 left by 1 */
	dbf		%d1,0b
	lsr.w   #8,%d0              /* Move ID to lower 8 bits of %d0 */

	cmp.b   #0xff,%d0
	bne.s   1f
	clr.b   %d0
1:
	cmp.l	#0,%a6
	beq.s	2f
	rte
2:	rts
