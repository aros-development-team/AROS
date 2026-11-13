/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Optimized 040+ CopyMem by Matt Hey
    Lang: english
*/

	#include "aros/m68k/asm.h"

#define SAFE_MOVE16 1

	.text
	.balign 4

	.globl	AROS_SLIB_ENTRY(CopyMem_040,Exec,104)
	.type	AROS_SLIB_ENTRY(CopyMem_040,Exec,104),@function
AROS_SLIB_ENTRY(CopyMem_040,Exec,104):

	subq.l #4,%d0			// size is 4 less than actual!
	bls.b smallcopy		        // if size<=4 bytes
	move.l %a1,%d1
	btst #0,%d1
	beq.b daligned2
	move.b (%a0)+,(%a1)+
	addq.l #1,%d1
	subq.l #1,%d0
daligned2:				// dest should be WORD aligned now
	btst #1,%d1
	beq.b daligned4
	move.w (%a0)+,(%a1)+
	subq.l #2,%d0
	bcs.b last2			// if size<0
daligned4:				// dest should be LONG aligned now
	cmp.l #512-4,%d0		// min size for move16, less than 252 is dangerous!
	bcc.b bigmove
move4loop:
	move.l (%a0)+,(%a1)+
	subq.l #4,%d0
	bhi.b move4loop		        // if size>0
	bne.b last2
move4:
	move.l (%a0)+,(%a1)+
	rts
smallcopy:
	beq.b move4
last2:
	btst #1,%d0
	beq.b last1
	move.w (%a0)+,(%a1)+
last1:
	btst #0,%d0
	bne.b move1
	rts
move1:
	move.b (%a0),(%a1)
	rts

	.balign 4
bigmove:
	sub.l #252,%d0			// make size 256 less than actual
	move.l %a1,%d1
	cmp.l #3072-256,%d0
	bcs.w bmove4loop
	btst #2,%d1			// destination aligned by 8 if bit3/bit#2=0
	beq.b	disal8			// if bit3/bit#2=0
	move.l (%a0)+,(%a1)+
	addq.l #4,%d1
	subq.l #4,%d0
disal8:
	btst #3,%d1			// destination aligned by 16 if bit4/bit#3=0
	beq.b	disal16			// if bit4/bit#3=0
	move.l (%a0)+,(%a1)+
	subq.l #8,%d0
	move.l (%a0)+,(%a1)+
disal16:
	move.l %a0,%d1
	and.w #15,%d1		// source aligned by 16 if first 4 bits=0
	bne.b bmove4loop		// if source not aligned by 16

#if SAFE_MOVE16
	cmp.l	#16777216,%a1	        // destination must be in 24 bit space
	bcs.b	bmove4loop
	cmp.l	#16777216,%a0	        // source must be in 24 bit space
	bcs.b	bmove4loop
#endif

move16loop:
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	sub.l	#256,%d0		// condition codes not affected by move16
	move16 (%a0)+,(%a1)+
	bcc.b	move16loop		// if d0>=0
	subq.b #4,%d0
	bcs.b l2
lloop:
	move.l (%a0)+,(%a1)+
	subq.b #4,%d0
	bcc.b lloop
l2:
	btst #1,%d0
	beq.b l1
	move.w (%a0)+,(%a1)+
l1:
	btst #0,%d0
	bne.b m1
	rts
	.balign 4
m1:
	move.b (%a0),(%a1)
	rts

	.balign 4
bmove4loop:				// move.l*64=64.l=256.b
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
	sub.l	#256,%d0
	bcc.w	bmove4loop		// if d0>=0
	subq.b #4,%d0
	bcs.b l22
lloop2:
	move.l (%a0)+,(%a1)+
	subq.b #4,%d0
	bcc.b lloop2
l22:
	btst #1,%d0
	beq.b l12
	move.w (%a0)+,(%a1)+
l12:
	btst #0,%d0
	bne.b m12
	rts
	.balign 4
m12:
	move.b (%a0),(%a1)
	rts
