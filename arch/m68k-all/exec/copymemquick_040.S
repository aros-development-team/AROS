/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 040+ Optimized CopyMemQuick by Matt Hey.
    Lang: english
*/

	#include "aros/m68k/asm.h"

#define UNALIGNEDWARNING 1
#define SAFE_MOVE16 1

	.text
	.balign 4

	.globl	AROS_SLIB_ENTRY(CopyMemQuick_040,Exec,105)
	.type	AROS_SLIB_ENTRY(CopyMemQuick_040,Exec,105),@function
AROS_SLIB_ENTRY(CopyMemQuick_040,Exec,105):

#if UNALIGNEDWARNING
	move.w	%d0,%d1
	and.w	#3,%d1
	beq.s	0f
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l	%d0,-(%sp)
	move.l	%a1,-(%sp)
	move.l	%a0,-(%sp)
	pea format040
	jsr kprintf
	lea	16(%sp),%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
0:
#endif

	subq.l #4,%d0		        // size is 4.b less than actual
	bls.b ism4or0		        // if d0<=0
prem4:
	cmp.l #512-4,%d0	        // min size for move16, less than 252 is dangerous!
	bcc.b bigmov
m4loop:
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	subq.l #8,%d0			// 8 less bytes to MOVE
	bhi.b m4loop			// if d0>0
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
	btst #2,%d1			// destination aligned by 8 if bit3/bit#2=0
	beq.b destisal8		        // if bit3/bit#2=0
	move.l (%a0)+,(%a1)+
	addq.l #4,%d1
	subq.l #4,%d0
destisal8:
	btst #3,%d1			// destination aligned by 16 if bit4/bit#3=0
	beq.b destisal16		// if bit4/bit#3=0
	move.l (%a0)+,(%a1)+
	subq.l #8,%d0
	move.l (%a0)+,(%a1)+
destisal16:
	move.l %a0,%d1
	and.w #15,%d1		// source aligned by 16 if first 4 bits=0
	bne.b bmov4loop		        // if source not aligned by 16

#if SAFE_MOVE16
	cmp.l #16777216,%a1	        // destination must be in 24 bit space
	bcs.b bmov4loop
	cmp.l #16777216,%a0	        // source must be in 24 bit space
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
	bcc.b	mov16loop		// if d0>=0
	subq.b #4,%d0
	bcc.b lastloop			// if d0>=0
	rts

	.balign 4
lastloop:
	move.l (%a0)+,(%a1)+
	subq.b #4,%d0
	bcc.b lastloop			// if d0>=0
	rts

	.balign 4
bmov4loop:				// move.l*64=64.l=256.b
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
	bcc.w	bmov4loop		// if d0>=0
	subq.b #4,%d0			// the byte is positive
	bcc.b lastloop2
	rts

	.balign 4
lastloop2:
	move.l (%a0)+,(%a1)+
	subq.b #4,%d0
	bcc.b lastloop2
	rts

#if UNALIGNEDWARNING
format040:
	.string "CopyMemQuick040(%p,%p,%08x) unaligned size\n"
#endif