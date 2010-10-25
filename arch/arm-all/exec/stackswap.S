/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

	#include "../include/aros/machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec)

AROS_SLIB_ENTRY(StackSwap,Exec):


	// r0 contains parameter 'newStack'
        // r1 contains ExecBase
	// lr contains the return address

	mov	r3, r0                  // newstack to r3	
	bl	get_offsetof_ThisTask   // result comes in r0
	ldr	r2, [r1, r0]            // r2 contains pointer to current task
	bl	get_offsetof_tc_SPLower // result comes in r0
	add	r2,  r2, r0             // r2 contains address of tc_SPLower

	/* Disable interrupts */
	stmdb	sp!, {r0, r1, r2, r3, lr}
	mov	r0, r1
	add	r0, r0, #Disable      // calculate function
	mov	lr, pc                // return address to lr (should add 4?)
	mov	pc, r0                // call Disable()
	ldmdb	sp!, {r0, r1, r2, r3, lr}

	/* Swap Lower boundaries */
	ldr	r0, [r2]              // r0 holds contents of tc_SPLower
	ldr	r1, [r3]              // r1 holds new contents of tc_SPLower
	str	r1, [r2], #4          // write new contents and postinc r2
	str	r0, [r3], #4          // save old contents back into passed struct 
                                      // postinc r3

	/* Swap higher boundaries */
	ldr	r0, [r2]              // r0 hold contents of tc_SPHigher
	ldr	r1, [r3]              // r1 holds new contents of tc_SPHigher
	str	r1, [r2]              // write new contents
	str	r0, [r3], #4          // save old contents back into passed struct
                                      // postinc r3

	/* Swap stackpointers */
	mov	r0, sp               // current stack pointer to r0
	ldr	sp, [r3]             // get desired stack pointer
	str	r0, [r3]             // save old stack pointer

	/* Enable interupts */
	stmdb	sp!, {r0, r1, r2, r3, lr}
	mov	r0, r1
	add	r1, r1, #Enable       // calculate function
	mov	lr, pc                // return address to lr (should add 4?)
	mov	pc, r0                // call Enable()
	ldmdb	sp!, {r0, r1, r2, r3, lr}

	/* Return from this function. There's no 'rts' instruction...*/
	mov	pc, lr
