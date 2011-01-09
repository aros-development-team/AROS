/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SuperstackSwap() - set up a new supervisor stack
    Lang: english

    This routine is expected to be called from Exec/Supervisor().

*/
/*****************************************************************************

    NAME
	SuperstackSwap

    LOCATION

    FUNCTION
        Sets up the new Supervisor stack

    INPUTS
	None.

    RESULT

    NOTES
        Must be called from Exec/Supervisor()

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
	#include "aros/m68k/asm.h"

/* On entry, the stack looks like:
 *   UWORD 0x0020  (only on 68010+)
 *   ULONG PC
 *   UWORD STATUS
 * A6 = SysBase
 * A5 = void SuperstackSwap(void)
 *
 * D0/D1 and A0/A1 can be used as scratch
 */
	.text
	.balign 4
	.globl SuperstackSwap
SuperstackSwap:
	move.l	%a6@(SysStkUpper),%a0
       moveq.l #-6,%d0
	btst	#0, %a6@(AttnFlags+1)	// If 68010+, copy frame ID
	beq	0f
	subq.l	#2,%d0			// Add 68010 space
0:
	lea.l	%a0@(%d0),%a1
1:
	move.w	%sp@+,%a0@(%d0)
	addq.l	#2,%d0
	bne  	1b
	move.l	%a1,%sp
	rte
