/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CopyMem
    Lang: english
*/

	#include "aros/m68k/asm.h"

	.text
	.balign 4

	.globl	AROS_SLIB_ENTRY(CopyMem,Exec,104)
	.type	AROS_SLIB_ENTRY(CopyMem,Exec,104),@function
AROS_SLIB_ENTRY(CopyMem,Exec,104):

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
