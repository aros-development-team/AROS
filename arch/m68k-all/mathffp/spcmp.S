/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: SPCmp.S

    Desc: mathffp/SPCmp m68k version. Generates correct 680x0 flags.
    Lang: english
*/

    #include "aros/m68k/asm.h"

    .text
    .balign 4
    .globl  AROS_SLIB_ENTRY(SPCmp,Mathffp,7)
AROS_SLIB_ENTRY(SPCmp,Mathffp,7):

	movem.l %d2,-(%sp)
	move.l	%d0,%d2

	moveq	#-1,%d0
	cmp.b	%d1,%d2
	bgt.s	done		/* d0 > d1 */
	moveq	#1,%d0
	cmp.b	%d1,%d2
	blt.s	done		/* d0 < d1 */
	cmp.l	%d1,%d2
	beq.s	done

	moveq	#-1,%d0
	cmp.l	%d1,%d2
	bgt.s	done		/* d0 > d1 */
	moveq	#1,%d0
	cmp.l	%d1,%d2
						/* d0 < d1 */
done:
	movem.l (%sp)+,%d2	/* must be MOVEM! Must not modify flags. */
	rts
