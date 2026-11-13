/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Optimized 060+ CopyMem by Matt Hey
    Lang: english
*/

	#include "aros/m68k/asm.h"

#define SAFE_MOVE16 1

	.text
	.balign 4

	.globl	AROS_SLIB_ENTRY(CopyMem_060,Exec,104)
	.type	AROS_SLIB_ENTRY(CopyMem_060,Exec,104),@function
AROS_SLIB_ENTRY(CopyMem_060,Exec,104):

	subq.l #4,%d0			// size is 4 less than actual!
	bls.b smallcopy		        // if size<=4 bytes
	move.l %a1,%d1
	lsr.l #1,%d1			// btst #0,d1
	bcc.b	daligned2		// beq.b daligned2
	move.b (%a0)+,(%a1)+
	addq.l #1,%d1
	subq.l #1,%d0
daligned2:				// dest should be WORD aligned now
	lsr.l #1,%d1			// btst #1,d1
	bcc.b	daligned4		// beq.b daligned4
	move.w (%a0)+,(%a1)+
	subq.l #2,%d0
	bcs.b last2			// if size<0
daligned4:				// dest should be LONG aligned now
	cmp.l #2048-4,%d0		// min size for move16, less than 252 is dangerous!
	bcc.b bigmove
move4loop:
	move.l (%a0)+,(%a1)+
	subq.l #4,%d0
	bhi.b move4loop		        // if size>0
	bne.b last2
move4:
	move.l (%a0)+,(%a1)+
	rts
move1:
	move.b (%a0)+,(%a1)+
	rts
smallcopy:
	beq.b move4
last2:
	btst #1,%d0
	beq.b last1
	move.w (%a0)+,(%a1)+
last1:
	lsr.l #1,%d0			// btst #0,d0
	bcs.b move1			// bne.b move1
	rts

bigmove:
	move.l %a1,%d1
	lsr.l #3,%d1			// destination aligned by 8 if bit3/bit#2=0
	bcc.b disal8			// if bit3/bit#2=0
	move.l (%a0)+,(%a1)+
	addq.l #1,%d1			// addq.l #4,d1
	subq.l #4,%d0
disal8:
	lsr.l #1,%d1			// destination aligned by 16 if bit4/bit#3=0
	bcc.b disal16			// if bit4/bit#3=0
	move.l (%a0)+,(%a1)+
	subq.l #8,%d0
	move.l (%a0)+,(%a1)+
disal16:
	move.l %a0,%d1
	and.w #15,%d1
	bne.b move4loop		        // if source not aligned by 16

#if SAFE_MOVE16
	cmp.l #16777216,%a1	        // destination must be in 24 bit space
	bcs.b move4loop
	moveq #128-4,%d1
	cmp.l #16777216,%a0	        // source must be in 24 bit space
	bcs.b move4loop
#else
	moveq #128-4,%d1
#endif

	sub.l %d1,%d0			// size is now 128 less than actual
	addq.l #4,%d1			// d1=128=bytes to move per loop
move16loop:
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	sub.l %d1,%d0			// d0=d0-128
	bcc.b move16loop		// if d0>=0
	moveq #128-4,%d1
	add.l %d1,%d0			// d0=d0+124, size is actual-4
	bcs.b move4loop		        // if d0>=0
l2:
	btst #1,%d0
	beq.b l1
	move.w (%a0)+,(%a1)+
l1:
	lsr.l #1,%d0
	bcs.b move1
	rts
