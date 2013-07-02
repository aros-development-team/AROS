/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CopyMem
    Lang: english
*/

	#include "aros/m68k/asm.h"

	.text
	.balign 4

	.globl	AROS_SLIB_ENTRY(CopyMem_000,Exec,104)
	.type	AROS_SLIB_ENTRY(CopyMem_000,Exec,104),@function
AROS_SLIB_ENTRY(CopyMem_000,Exec,104):
	moveq #64,%d1
	cmp.l %d1,%d0
	bcs.s copy_00_bytes
	move.l %a0,%d1
	btst #0,%d1
	beq.s copy_00_salign
	move.b (%a0)+,(%a1)+
	subq.l #1,%d0
copy_00_salign:
	move.l %a1,%d1
	btst #0,%d1
	// alignment mismatch?
	bne.s copy_00_bytes
	movem.l %d2-%d7/%a2-%a6,-(%sp)
copy_00_movem:
	movem.l (%a0)+,%d1-%d7/%a2-%a6
	movem.l %d1-%d7/%a2-%a6,(%a1)
	moveq #12*4,%d1
	sub.l %d1,%d0
	add.l %d1,%a1
	cmp.l %d1,%d0
	bcc.s copy_00_movem
	movem.l (%sp)+,%d2-%d7/%a2-%a6
	// copy byte by byte
copy_00_bytes:
	move.w %d0,%d1
	swap %d0
	bra.s copy_00_bytes2
copy_00_bytes_loop:
	move.b (%a0)+,(%a1)+
copy_00_bytes2:
	dbf %d1,copy_00_bytes_loop
	dbf %d0,copy_00_bytes_loop
	rts	

	/* Optimized 020+ CopyMem by Matt Hey */

	.balign 4
	.globl	AROS_SLIB_ENTRY(CopyMem_020,Exec,104)
	.type	AROS_SLIB_ENTRY(CopyMem_020,Exec,104),@function
AROS_SLIB_ENTRY(CopyMem_020,Exec,104):
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
	sub.l %d1,%d0				// size is now 128 less than actual
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
	sub.l	%d1,%d0				// d0=d0-128
	bcc.b	bigmove4			// if d0>=0
	add.l %d1,%d0				// d0=d0+128, back to positive
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
