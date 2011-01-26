/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheControl() - Global control of the system caches.
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_LH2(ULONG, CacheControl,

    SYNOPSIS
	AROS_LHA(ULONG, cacheBits, D0),
	AROS_LHA(ULONG, cacheMask, D1),

    LOCATION
	struct ExecBase *, SysBase, 108, Exec)

    FUNCTION
	This function will provide global control of all the processor
	instruction and data caches. It is not possible to have per
	task control.

	The actions undertaken by this function are very CPU dependant,
	however the actions performed will match the specified options
	as close as is possible.

	The commands currently defined in the include file exec/execbase.h
	are closely related to the cache control register of the Motorola
	MC68030 CPU.

    INPUTS
	cacheBits   -   The new state of the bits
	cacheMask   -   A mask of the bits you wish to change.

    RESULT
	oldBits     -   The complete value of the cache control bits
			prior to the call of this function.

	Your requested actions will have been performed. As a side effect
	this function will also cause the caches to be cleared.

    NOTES
	On CPU's without a separate instruction and data cache, these will
	be considered as equal.

    EXAMPLE

    BUGS

    SEE ALSO
	CacheClearE(), CacheClearU()

    INTERNALS
	This function requires replacing in $(KERNEL), or possibly
	even $(ARCH) in some cases.

******************************************************************************/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.chip 68020
	.globl	AROS_SLIB_ENTRY(CacheControl,Exec)
AROS_SLIB_ENTRY(CacheControl,Exec):

    movem.l %d2/%d3/%a5,%sp@-
    move.l %d0,%d2
    moveq #0,%d0
    move.w %a6@(AttnFlags),%d3
    and.w #0x008E,%d3 // 020/030/040/060?
    beq.s 0f
    and.w #0x0088,%d3 // 040/060?
    beq.s 2f

	and.l #0x0101,%d1
	and.l #0x0101,%d2
	// code cache 0 -> 15
    bclr #0,%d1
    beq.s 3f
    or.l #0x20808000,%d1
3:	bclr #0,%d2
	beq.s 4f
    or.l #0x20808000,%d2
4:  
    // data cache 8 -> 31
    bclr #8,%d1
    beq.s 5f
    bset #31,%d1
5:	bclr #8,%d2
	beq.s 6f
	bset #31,%d2
6:

2:	lea su(%pc),%a5
	jsr Supervisor(%a6)

    move.w %a6@(AttnFlags),%d3
    and.w #0x0088,%d3 // 040/060?
    beq.s 0f
    move.l %d0,%d1
    moveq #0,%d0
    btst #15,%d1
    beq.s 1f
    // code+burst+write-allocate
    or.w #0x2011,%d0
1:	btst #31,%d1
	beq.s 0f
	// data+burst+copyback
	or.l #0x80002100,%d0
0:
	movem.l %sp@+,%d2/%d3/%a5
    rts

su:	dc.l 0x4e7a0002 //movec %cacr,%d0
	move.l %d0,%d3
	and.l %d1,%d2
	not.l %d1
	and.l %d1,%d3
	or.l %d2,%d3
	dc.l 0x4e7b3002 // movec %d3,%cacr
	rte
