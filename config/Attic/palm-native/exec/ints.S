/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

	.text
	.balign 16
	.globl	_sys_dispatch

	/* asm entry of system task dispatcher */
_sys_dispatch:
	/* mask all interrupts while I am doing this here */
	move.l	#-1,0xfffff304

	movem.l	%d0-%d7/%a0-%a6,-(%ssp)
	move.l	%usp,%a0
	movem.l	%a0,-(%ssp)
	move.l	%ssp,%a0
	move.l	%a0,-(%ssp)
	bsr	sys_Dispatch
	add.l	#4,%ssp
	movem.l	(%ssp)+,%a0
	move.l	%a0,%usp
	movem.l	(%ssp)+,%d0-%d7/%a0-%a6

	/* clear the interrupt bits for the 1 & 512Hz interrupts */
	move.w	#0x8010,0xfffffb0e
	/* allow the RTC interrupts again */
	move.l	#0xffbfffef,0xfffff304
	rte


