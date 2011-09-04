/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/
        
	#include <aros/arm/asm.h>

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(NewStackSwap,Exec,134)

AROS_SLIB_ENTRY(NewStackSwap,Exec,134):

	// r0 contains parameter 'newStack'
        // r1 contains new entry point
        // r2 contains argument array
        // r3 contains SysBase
	// lr contains the return address

	stmfd	sp!, {r4, r5, r6, r7, lr}
	
	// Store all parameters
	mov	r4, r0
	mov	r5, r1
	mov	r6, r2
	mov	r7, r3

#error implement following (pseudo) C code
        // oldtop = aros_get_altstack(SysBase->ThisTask)
        	
	// Call StackSwap
	mov	r1, r3
	ldr	ip, [r1, StackSwap]
	blx	ip

        // Call aros_init_altstack
#error implement following (pseudo) C code
        // aros_init_altstack(SysBase->ThisTask)
        // aros_set_altstack(SysBase->ThisTask, oldtop)
        
	// Prepare first 4 arguments
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	ldr	r2, [r6], #4
	ldr	r3, [r6], #4
	sub	sp, sp, #4*4
	// Prepare last 4 arguments on the stack
	ldr	ip, [r6], #4
	str	ip, [sp, #0]
	ldr	ip, [r6], #4
	str	ip, [sp, #4]
	ldr	ip, [r6], #4
	str	ip, [sp, #8]
	ldr	ip, [r6], #4
	str	ip, [sp, #12]
	
	// Jump to subroutine with new stack and arguments
	blx	r5
	// Put return value to r5
	mov	r5, r0
	
	add	sp, sp, #4*4
	
	// Call StackSwap again
	mov	r0, r4
	mov	r1, r7
	ldr	ip, [r1, StackSwap]
	blx	ip
	
	// Restore return value
	mov	r0, r5
	ldmfd	sp!, {r4, r5, r6, r7, lr}
	bx	lr

