/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

	#include <aros/armeb/asm.h>

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec,122)

AROS_SLIB_ENTRY(StackSwap,Exec,122):

	// r0 contains parameter 'newStack'
        // r1 contains ExecBase
	// lr contains the return address

	stmfd sp!, {r0, r1, lr}     // save some useful registers
	mov r0, #0
	ldr ip, [r1, #-196]         // Call FindTask(NULL)
	blx ip
	mov r2, r0                  // copy result to r2 register
	ldmfd sp!, {r0, r1, lr}     // restore r0, r1 and lr
	mov	r3, r0                  // newstack to r3	

	/* Disable interrupts */
	stmfd	sp!, {r0, r1, r2, r3, lr}
	ldr	ip, [r1, Disable]     // ip = GETVECADDR(Disable)
	mov	r0, r1                // One argument = SysBase
	blx	ip
	ldmfd	sp!, {r0, r1, r2, r3, lr}

	/* Swap Lower boundaries, use ip as scratch register (r1 holds SysBase!!!!) */
	ldr	r0, [r2, tc_SPLower]  // r0 holds old contents of tc_SPLower
	ldr	ip, [r3, stk_Lower]   // ip holds new contents of tc_SPLower
	str	ip, [r2, tc_SPLower]  // write new contents
	str	r0, [r3, stk_Lower]   // save old contents back into passed struct 
                                      // postinc r3

	/* Swap higher boundaries */
	ldr	r0, [r2, tc_SPUpper] // r0 holds old contents of tc_SPHigher
	ldr	ip, [r3, stk_Upper]  // ip holds new contents of tc_SPHigher
	str	ip, [r2, tc_SPUpper] // write new contents
	str	r0, [r3, stk_Upper]  // save old contents back into passed struct

	/* Swap stackpointers */
	mov	r0, sp                // current stack pointer to r0
	ldr	sp, [r3, stk_Pointer] // get desired stack pointer
	str	r0, [r3, stk_Pointer] // save old stack pointer

	/* Enable interupts */
	stmfd	sp!, {r0, r1, lr}
	ldr	ip, [r1, Enable]      // ip = GETVECADDR(Enable)
	mov	r0, r1                // One argument = SysBase
	blx	ip
	ldmfd	sp!, {r0, r1, lr}

	/* Return from this function. There's no 'rts' instruction...*/
	bx	lr
