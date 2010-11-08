/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

	#include <aros/arm/asm.h>

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec)

AROS_SLIB_ENTRY(StackSwap,Exec):

	// r0 contains parameter 'newStack'
        // r1 contains ExecBase
	// lr contains the return address

	mov	r3, r0                  // newstack to r3	
	ldr	r2, [r1, ThisTask]      // r2 contains pointer to current task
	add	r2,  r2, tc_SPLower     // r2 contains address of tc_SPLower

	/* Disable interrupts */
	stmfd	sp!, {r0, r1, r2, r3, lr}
	ldr	ip, [r1, Disable]     // ip = GETVECADDR(Disable)
	mov	r0, r1                // One argument = SysBase
	blx	ip
	ldmfd	sp!, {r0, r1, r2, r3, lr}

	/* Swap Lower boundaries, use ip as scratch register (r1 holds SysBase!!!!) */
	ldr	r0, [r2]              // r0 holds contents of tc_SPLower
	ldr	ip, [r3]              // ip holds new contents of tc_SPLower
	str	ip, [r2], #4          // write new contents and postinc r2
	str	r0, [r3], #4          // save old contents back into passed struct 
                                      // postinc r3

	/* Swap higher boundaries */
	ldr	r0, [r2]              // r0 hold contents of tc_SPHigher
	ldr	ip, [r3]              // r1 holds new contents of tc_SPHigher
	str	ip, [r2]              // write new contents
	str	r0, [r3], #4          // save old contents back into passed struct
                                      // postinc r3

	/* Swap stackpointers */
	mov	r0, sp               // current stack pointer to r0
	ldr	sp, [r3]             // get desired stack pointer
	str	r0, [r3]             // save old stack pointer

	/* Enable interupts */
	stmfd	sp!, {r0, r1, r2, r3, lr}
	ldr	ip, [r1, Enable]      // ip = GETVECADDR(Enable)
	mov	r0, r1                // One argument = SysBase
	blx	ip
	ldmfd	sp!, {r0, r1, r2, r3, lr}

	/* Return from this function. There's no 'rts' instruction...*/
	bx	lr

