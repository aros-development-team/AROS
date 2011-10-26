/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheClearE_00,Exec,107)
	.type	AROS_SLIB_ENTRY(CacheClearE_00,Exec,107),@function
AROS_SLIB_ENTRY(CacheClearE_00,Exec,107):
	rts

	.globl	AROS_SLIB_ENTRY(CacheClearE_20,Exec,107)
	.type	AROS_SLIB_ENTRY(CacheClearE_20,Exec,107),@function
AROS_SLIB_ENTRY(CacheClearE_20,Exec,107):
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
	.type	AROS_SLIB_ENTRY(CacheClearE_460,Exec,107),@function
AROS_SLIB_ENTRY(CacheClearE_4060,Exec,107):
	move.l	%a5,%a1
	lea	cacheclearesup_4060,%a5
	jmp	Supervisor(%a6)

cacheclearesup_4060:
	move.l	%a1,%a5
	/* both, code or data cache bits set? */
  and.w #0x0808,%d1
  beq.s 0f
  /* just flush whole cache if region is big enough */
  cmp.l #0x00100000,%d0
  bhi.s 4f
  move.l %d2,%a1
  add.l %a0,%d0
  move.l %a0,%d2
  and.w #~4095,%d2
  move.l %d2,%a0
  move.l %a1,%d2
  cmp.w #0x0808,%d1
  beq.s 1f
  cmp.w #0x0008,%d1
  beq.s 2f
  /* data only */
3: nop
  cpushp %dc,(%a0)
  lea 4096(%a0),%a0
  cmp.l %a0,%d0
  bhi.s 3b
  bra.s 0f
  /* code only */
2: nop
  cpushp %ic,(%a0)
  lea 4096(%a0),%a0
  cmp.l %a0,%d0
  bhi.s 2b
  bra.s 0f
  /* both */
1: nop
  cpushp %bc,(%a0)
  lea 4096(%a0),%a0
  cmp.l %a0,%d0
  bhi.s 1b
0: rte  

  /* flush whole cache: both, data only or code only */
4:
  cmp.w #0x0808,%d1
  beq.s 5f
  cmp.w #0x0008,%d1
  beq.s 1f
	nop
	cpusha	%dc
	bra.s 0b
1: nop
	cpusha %ic
	bra.s 0b
5: nop
	cpusha	%bc
	bra.s 0b
