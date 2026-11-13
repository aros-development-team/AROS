/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Optimized 020+ CopyMem by Matt Hey
    Lang: english
*/

	#include "aros/m68k/asm.h"

	.text
	.balign 4

	.globl	AROS_SLIB_ENTRY(CopyMem_020,Exec,104)
	.type	AROS_SLIB_ENTRY(CopyMem_020,Exec,104),@function
AROS_SLIB_ENTRY(CopyMem_020,Exec,104):

	subq.l #4,%d0			// size is 4 less than actual!
	bls.b smallcopy		        // if size<=4 bytes
	move.l %a1,%d1
	btst #0,%d1
	beq.b daligned2
	move.b (%a0)+,(%a1)+
	addq.l #1,%d1
	subq.l #1,%d0
daligned2:					// dest should be WORD aligned now
	btst #1,%d1
	beq.b daligned4
	move.w (%a0)+,(%a1)+
	subq.l #2,%d0
	bcs.b last2				// if size<0
daligned4:					// dest should be LONG aligned now
	cmp.l #256-4,%d0		// min size for move16, less than 252 is dangerous!
	bcc.b bigmove
move4loop:
	move.l (%a0)+,(%a1)+
	subq.w #4,%d0
	bhi.b move4loop		// if size>0
smallcopy:
	bne.b last2
move4:
	move.l (%a0),(%a1)
	rts

	.balign 4
bigmove:
	moveq #128-4,%d1
	sub.l %d1,%d0			// size is now 128 less than actual
	addq.l #4,%d1			// d1=128=bytes to move per loop
bigmove4:
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	sub.l	%d1,%d0			// d0=d0-128
	bcc.b	bigmove4			// if d0>=0
	add.l %d1,%d0			// d0=d0+128, back to positive
	subq.l #4,%d0
	bcs.b last2
lloop:
	move.l (%a0)+,(%a1)+
	subq.w #4,%d0
	bcc.b lloop
last2:
	btst #1,%d0
	beq.b last1
	move.w (%a0)+,(%a1)+
last1:
	btst #0,%d0
	bne.b move1
	rts

	.balign 4
move1:
	move.b (%a0),(%a1)
	rts
