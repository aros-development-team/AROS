/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: SPTst.S

    Desc: mathffp/SPTst m68k version. Generates correct 680x0 flags.
    Lang: english
*/

    #include "aros/m68k/asm.h"

    .text
    .balign 4
    .globl  AROS_SLIB_ENTRY(SPTst,Mathffp,8)
    .globl  AROS_SLIB_ENTRY(SPCmp,Mathffp,7)
AROS_SLIB_ENTRY(SPTst,Mathffp,8):

	moveq	#0,%d0
	/* == 0? */
	tst.b	%d1
	beq.s	.done
	/* >0? */
	moveq	#1,%d0
	tst.b	%d1
	bpl.s	.done
	/* <0 */
	moveq	#-1,%d0
	tst.b	%d1
.done:
	rts
