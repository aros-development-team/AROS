/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id: $

    Lang: english
*/

	#include "aros/m68k/asm.h"

#define UNALIGNEDWARNING 1

	.text
	.balign 4

	.globl	AROS_SLIB_ENTRY(CopyMemQuick,Exec,105)
	.type	AROS_SLIB_ENTRY(CopyMemQuick,Exec,105),@function
AROS_SLIB_ENTRY(CopyMemQuick,Exec,105):

#if UNALIGNEDWARNING
	move.w	%d0,%d1
	and.w	#3,%d1
	beq.s	0f
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l	%d0,-(%sp)
	move.l	%a1,-(%sp)
	move.l	%a0,-(%sp)
	pea format
	jsr kprintf
	lea	16(%sp),%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
0:
#endif

	lsr.l	#2,%d0
	beq.s	2f

	/* TODO: Optimize big copies */

1:	subq.l	#1,%d0
	move.l	%d0,%d1
	swap	%d1
3:	move.l	(%a0)+,(%a1)+
	dbf		%d0,3b
	dbf		%d1,3b
2:
	rts

#if UNALIGNEDWARNING
format:
	.string "CopyMemQuick(%p,%p,%08x) unaligned size\n"
#endif