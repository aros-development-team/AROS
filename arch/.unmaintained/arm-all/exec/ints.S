/*  Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

	.text
	.balign 16
	.globl	_sys_dispatch

	/* asm entry of system task dispatcher */
_sys_dispatch:
	
	stmdb	sp!,{r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,sp,lr}^
	mrs	r0, SPSR
	stmdb	sp!, {r0}
	                                    // Save context onto stack,
	                                    // What's on the stack looks
	                                    // like the pt_regs structure from ptrace.h
	mov	r0, sp                      // r0 contains address of pt_regs
	bl	sys_Dispatch
	ldmdb	sp!, {r0}
	msr	SPSR, r0
	ldmdb	sp!,{r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,sp,lr}^

	/* Return from this function. There's no 'rts' instruction...*/
	/* the following function automatically restores pc and cpsr */
	subs	pc, lr, #4
