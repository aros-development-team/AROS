/*
    Copyright © 2017-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Optimized Apollo Core 080 CopyMem by Bax
    Lang: english
*/

	#include "aros/m68k/asm.h"

	.text
	.balign 4

	.globl	AROS_SLIB_ENTRY(CopyMem_ac080,Exec,104)
	.type	AROS_SLIB_ENTRY(CopyMem_ac080,Exec,104),@function
AROS_SLIB_ENTRY(CopyMem_ac080,Exec,104):

	move.l %d3,-(%sp)

	cmp.l #8,%d0		// max. 8 left, 7 right and at least 8 in the middle (the latter two are checked against 0)
	blt blit_up_slow

	//
	//
	// phase 1: check dest alignment and make sure it's aligned
	//
	//
	move.l %a1,%d3	// dest pointer
	and.l	#7,%d3		// 0..7

	moveq #-8,%d1	// -8
	add.l	%d0,%d1		// width minus maxbytes(phase1)

	add.l	%d3,%d1		// width-phase1 (subtracts -8 -7 -6 -5 -4 -3 -2 -1)

	move.l %d1,%d0	// bytes remaining after phase 1
	lsr.l #3,%d0		// remaining bytes / 8 (for main phase)
	and #7,%d1		// leftover bytes for phase 3

	// %d0 main loop in bytes/8
	// %d1 leftover bytes
	jmp	blit_up_fast_xoffs(%pc,%d3.w*2)
blit_up_fast_xoffs:
	bra.s	blit_up_fast_xoff0
	bra.s	blit_up_fast_xoff1
	bra.s	blit_up_fast_xoff2
	bra.s	blit_up_fast_xoff3
	bra.s	blit_up_fast_xoff4
	bra.s	blit_up_fast_xoff5
	bra.s	blit_up_fast_xoff6
	bra.s	blit_up_fast_xoff7
blit_up_fast_xoff0:
	// aligned already (yay!)
	move.l (%a0)+,(%a1)+	//copy 8 bytes in pre phase
	move.l (%a0)+,(%a1)+
	bra.s	blit_up_fast_xmain
blit_up_fast_xoff1:
	move.b (%a0)+,(%a1)+
	move.w (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	bra.s	blit_up_fast_xmain
blit_up_fast_xoff2:
	move.w (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	bra.s	blit_up_fast_xmain
blit_up_fast_xoff3:
	move.b (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
	bra.s	blit_up_fast_xmain
blit_up_fast_xoff4:
	move.l (%a0)+,(%a1)+
	bra.s	blit_up_fast_xmain
blit_up_fast_xoff5:
	move.b (%a0)+,(%a1)+
	move.w (%a0)+,(%a1)+
	bra.s	blit_up_fast_xmain
blit_up_fast_xoff6:
	move.w (%a0)+,(%a1)+
	bra.s	blit_up_fast_xmain
blit_up_fast_xoff7:
	move.b (%a0)+,(%a1)+
	bra.s	blit_up_fast_xmain

	//
	//
	// phase 2: main copy loop (8 Bytes)
	//
	//
blit_up_fast_xloop:
	move.l (%a0)+,(%a1)+
	move.l (%a0)+,(%a1)+
blit_up_fast_xmain:
	subq.l #1,%d0
	bpl.b	blit_up_fast_xloop

	bra.s	blit_up_fast_xleftmain

	//
	//
	// phase 3: copy remainder ...
	//
	//
blit_up_fast_xleftloop:
	move.b (%a0)+,(%a1)+
blit_up_fast_xleftmain:
	dbf %d1,blit_up_fast_xleftloop

blit_up_fast_xend:

	bra.s	end

	// this loop is generic, but slow. It is called here when the copied width 
	// is small.
blit_up_slow:
	// A0 - Src Pointer
	// A1 - Dst Pointer
	// %d0 - Size (in Bytes)

	subq.l	#1,%d0
blit_up_xloop:
	move.b	(%a0)+,(%a1)+
	dbf	%d0,blit_up_xloop			// dbf is ok, we come here for small lengths only

end:
	movem.l	(%sp)+,%d3
	rts
