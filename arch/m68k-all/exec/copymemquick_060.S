/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 060+ Optimized CopyMemQuick by Matt Hey.
    Lang: english
*/

	#include "aros/m68k/asm.h"

#define UNALIGNEDWARNING 1
#define SAFE_MOVE16 1

	.text
	.balign 16

	.globl	AROS_SLIB_ENTRY(CopyMemQuick_060,Exec,105)
	.type	AROS_SLIB_ENTRY(CopyMemQuick_060,Exec,105),@function
AROS_SLIB_ENTRY(CopyMemQuick_060,Exec,105):

#if UNALIGNEDWARNING
	move.w	%d0,%d1
	and.w	#3,%d1
	beq.s	0f
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l	%d0,-(%sp)
	move.l	%a1,-(%sp)
	move.l	%a0,-(%sp)
	pea format060
	jsr kprintf
	lea	16(%sp),%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
0:
#endif
	subq.l #4,%d0		        // size is 4.b less than actual
	bls.b ism4or0		        // if d0<=0
prem4:
	cmp.l #2048-4,%d0	        // min size for move16, less than 252 is dangerous!
	bcc.b bigmov
m4loop:
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	subq.l #8,%d0			// 8 less bytes to MOVE
	bhi.b m4loop			// if d0>0
	beq.b lastm4
	rts
lastm4:
	move.l (%a0)+,(%a1)+
	rts
ism4or0:
	beq.b lastm4
	rts

lastloop:
	move.l (%a0)+,(%a1)+
	subq.l #4,%d0
	bhi.b lastloop			// if d0>0
	rts

bigmov:
	move.l %a1,%d1
	lsr.l #3,%d1			// destination aligned by 8 if bit3/bit#2=0
	bcc.b destisal8		        // if bit3/bit#2=0
	move.l (%a0)+,(%a1)+
	addq.l #1,%d1			// addq.l #4,d1
	subq.l #4,%d0
destisal8:
	lsr.l #1,%d1			// destination aligned by 16 if bit4/bit#3=0
	bcc.b destisal16		// if bit4/bit#3=0
	move.l (%a0)+,(%a1)+
	subq.l #8,%d0
	move.l (%a0)+,(%a1)+
destisal16:
	move.l %a0,%d1
	and.w #15,%d1
	bne.b m4loop			// if source not aligned by 16

#if SAFE_MOVE16
	cmp.l	#16777216,%a1	        // destination must be in 24 bit space
	bcs.b	m4loop
	moveq #128-4,%d1
	cmp.l	#16777216,%a0	        // source must be in 24 bit space
	bcs.b	m4loop
#else
	moveq #128-4,%d1	
#endif

	sub.l %d1,%d0			// size is now 128 less than actual
	addq.l #4,%d1			// d1=128=bytes to move per loop
mov16loop:
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	move16 (%a0)+,(%a1)+
	sub.l	%d1,%d0			// d0=d0-128
	bcc.b	mov16loop		// if d0>=0
	add.l %d1,%d0			// d0=d0+128, back to positive
	bne.b lastloop
	rts

#if UNALIGNEDWARNING
format060:
	.string "CopyMemQuick060(%p,%p,%08x) unaligned size\n"
#endif