/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

	#include "aros/m68k/asm.h"
	#include "cache.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheClearE_00,Exec,107)
	.type	AROS_SLIB_ENTRY(CacheClearE_00,Exec,107),@function
AROS_SLIB_ENTRY(CacheClearE_00,Exec,107):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	rts

	.globl	AROS_SLIB_ENTRY(CacheClearE_20,Exec,107)
	.type	AROS_SLIB_ENTRY(CacheClearE_20,Exec,107),@function
AROS_SLIB_ENTRY(CacheClearE_20,Exec,107):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	move.l	%a5,%a1
	lea.l	cacheclearesup_20,%a5
	jmp	Supervisor(%a6)

cacheclearesup_20:
	and.w #0x0808,%d1
	beq.s 0f
	or.w	#0x0700,%sr	/* Disable interrupts so cacr can not be influenced
				   while we clear the caches */
	movec	%cacr,%d0
	or.w	%d1,%d0
	movec	%d0,%cacr
0:
	move.l	%a1,%a5		/* Restore a5 */
	rte			/* This rte will restore the SR from the stack */

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheClearE_4060,Exec,107)
	.type	AROS_SLIB_ENTRY(CacheClearE_4060,Exec,107),@function
AROS_SLIB_ENTRY(CacheClearE_4060,Exec,107):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	move.l	%a5,%a1
	lea	cacheclearesup_4060,%a5
	jmp	Supervisor(%a6)

/* CACRF_ClearI = flush both caches (new code may still be only in data cache)
 * CACRF_ClearD = flush data cache only
 */

cacheclearesup_4060:
	move.l	%a1,%a5
#if CACHEFULLFLUSH == 0
	/* just flush whole cache if region is big enough */
	cmp.l #0x00100000,%d0
	bhi.s 4f
	move.l %d2,%a1
	add.l %a0,%d0
	move.l %a0,%d2
	and.w #~4095,%d2
	move.l %d2,%a0
	move.l %a1,%d2
	/* CACRF_ClearI set? */
	btst #3,%d1
	bne.s 2f
	/* data only */
3:	nop
	cpushp %dc,(%a0)
	lea 4096(%a0),%a0
	cmp.l %a0,%d0
	bhi.s 3b
	bra.s 0f
	/* both */
2:	nop
	cpushp %bc,(%a0)
	lea 4096(%a0),%a0
	cmp.l %a0,%d0
	bhi.s 2b
0:	rte
4:
#endif

	/* flush whole cache: data only or both */
	/* CACRF_ClearI set? */
	btst #3,%d1
	bne.s 1f
	nop
	cpusha %dc
	rte
1:	nop
	cpusha %bc
	rte


#if CACHEDEBUG
cachedebug:	
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l	%d1,-(%sp)
	move.l	%d0,-(%sp)
	move.l	%a0,-(%sp)
	pea format
	jsr kprintf
	lea	16(%sp),%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts
format:
	.string "CE(%08x,%08x,%08x)\n"
#endif
