/*  Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: ints.S 15214 2002-08-15 02:28:46Z bergers $
*/

	.text
	.balign 16
	.globl	_sys_dispatch

	/* asm entry of system task dispatcher */
_sys_dispatch:

	movem.l	%d0-%d7/%a0-%a6,-(%ssp)     // Save context onto stack,
	                                    // exception context already saves PC and SR
	                                    // What's on the stack looks
	                                    // like the pt_regs structure from ptrace.h
	move.l	%usp,%a0
	movem.l	%a0,-(%ssp)
	move.l	%ssp,%a0
	move.l	%a0,-(%ssp)                 // A0 contains address of pt_regs
	bsr	sys_Dispatch
	add.l	#4,%ssp
	movem.l	(%ssp)+,%a0
	move.l	%a0,%usp
	movem.l	(%ssp)+,%d0-%d7/%a0-%a6

	/* 
	   clear the LO IRQ status register!! POSE has a bug here for
	   clearing all of it at once (HI+LO)
	*/
//	clr.w  0xfffff30e     // clear the IRQ status register LO
	rte
