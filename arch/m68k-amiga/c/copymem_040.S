/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: 68040 optimized CopyMem/Quick by Matt Hey.
    Lang: english
*/

	.text
	.chip 68040

#define SAFE_MOVE16 0

/***********************************
 *
 * CopyMemQuick( source, dest, size )
 *                 %a0     %a1    %d0
 *
 *	source & dest = long word aligned
 *	size = in bytes (divisable by 4)
 *
 */
	.globl copymemquick_040
	.globl copymemquicke_040
	.balign 4
copymemquick_040:
	subq.l #4,%d0		// size is 4.b less than actual
	bls.b ism4or0		// if %d0<=0
prem4:
	cmp.l #512-4,%d0	// min size for move16, less than 252 is dangerous!
	bcc.b bigmov
m4loop:
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	subq.l #8,%d0			// 8 less bytes to MOVE
	bhi.b m4loop			// if %d0>0
ism4or0:
	beq.b lastm4
	rts
lastm4:
	move.l (%a0),(%a1)
	rts

	.balign 4
bigmov:
	sub.l #252,%d0			// make size 256 less than actual
	move.l %a1,%d1
	cmp.l #3072-256,%d0
	bcs.w bmov4loop
	btst #2,%d1				// destination aligned by 8 if bit3/bit#2=0
	beq.b destisal8		// if bit3/bit#2=0
	move.l (%a0)+,(%a1)+
	addq.l #4,%d1
	subq.l #4,%d0
destisal8:
	btst #3,%d1				// destination aligned by 16 if bit4/bit#3=0
	beq.b destisal16		// if bit4/bit#3=0
	move.l (%a0)+,(%a1)+
	subq.l #8,%d0
	move.l (%a0)+,(%a1)+
destisal16:
	move.l %a0,%d1
	and.w #15,%d1		// source aligned by 16 if first 4 bits=0
	bne.b bmov4loop		// if source not aligned by 16

#if SAFE_MOVE16
	cmp.l #$01000000,a1	; destination must be in 24 bit space
	bcs.b bmov4loop
	cmp.l #$01000000,a0	; source must be in 24 bit space
	bcs.b bmov4loop
#endif

mov16loop:
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
	sub.l #256,%d0			// condition codes not affected by move16
	move16 (%a0)+,(%a1)+
	bcc.b	mov16loop		// if %d0>=0
	subq.b #4,%d0
	bcc.b lastloop			// if %d0>=0
	rts
	.balign 4
lastloop:
	move.l (%a0)+,(%a1)+
	subq.b #4,%d0
	bcc.b lastloop			// if %d0>=0
	rts

	.balign 4
bmov4loop:					//move.l*64=64.l=256.b
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
	bcc.w	bmov4loop		// if %d0>=0
	subq.b #4,%d0			// the byte is positive
	bcc.b lastloop2
	rts
	.balign 4
lastloop2:
	move.l (%a0)+,(%a1)+
	subq.b #4,%d0
	bcc.b lastloop2
	rts
copymemquicke_040:

/**********************************
 *
 * CopyMem( source, dest, size )
 *            %a0     %a1    %d0
 *
 *	source & dest = any aligned
 *	size = in bytes
 *
 */

	.balign 4
	.globl copymem_040
	.globl copymeme_040
copymem_040:
	subq.l #4,%d0			// size is 4 less than actual!
	bls.b smallcopy		// if size<=4 bytes
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
	cmp.l #512-4,%d0		// min size for move16, less than 252 is dangerous!
	bcc.b bigmove
move4loop:
	move.l (%a0)+,(%a1)+
	subq.l #4,%d0
	bhi.b move4loop		// if size>0
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
	btst #2,%d1				// destination aligned by 8 if bit3/bit#2=0
	beq.b	disal8			// if bit3/bit#2=0
	move.l (%a0)+,(%a1)+
	addq.l #4,%d1
	subq.l #4,%d0
disal8:
	btst #3,%d1				// destination aligned by 16 if bit4/bit#3=0
	beq.b	disal16			// if bit4/bit#3=0
	move.l (%a0)+,(%a1)+
	subq.l #8,%d0
	move.l (%a0)+,(%a1)+
disal16:
	move.l %a0,%d1
	and.w #15,%d1		// source aligned by 16 if first 4 bits=0
	bne.b bmove4loop		// if source not aligned by 16

#if SAFE_MOVE16
	cmp.l	#$01000000,a1	; destination must be in 24 bit space
	bcs.b	bmove4loop
	cmp.l	#$01000000,a0	; source must be in 24 bit space
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
	sub.l	#256,%d0			// condition codes not affected by move16
	move16 (%a0)+,(%a1)+
	bcc.b	move16loop		// if %d0>=0
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
bmove4loop:					//move.l*64=64.l=256.b
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
	bcc.w	bmove4loop		// if %d0>=0
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
copymeme_040:
