/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

/******************************************************************************

    NAME

	aros_thunk_relbase

    FUNCTION
	Save the current relbase, and call the function at the LVO
	specified.

    INPUTS

    RESULT
	None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/


#include "aros/x86_64/asm.h"

	// Don't change %rsp just store extra data at a negative offset from 
	// the pointer
#define LIBBASE -8
#define FUNCVEC -16
#define R13STORE -24
#define TMP -32
		
	.text
	.balign 4

	.globl	aros_thunk_libfunc

	// %rsp(+12..) - Arguments to called function
	// %rsp@(0) - Return address
	// r10 - New base address
	// r11 - LVO offset

aros_thunk_libfunc:
	movq	%r13,R13STORE(%rsp)
	movq	%r10,LIBBASE(%rsp)
	addq	%r10,%r11
	movq	%r11,FUNCVEC(%rsp)
	
	// Get task's tc_SPLower
	movq	SysBase(%rip), %r13
	movq	ThisTask(%r13),%r13
	movq	tc_SPLower(%r13),%r13

	// Get relstack pointer
	movq	(%r13),%r10	// %r10 is now relstack pointer

	// r10          - Relstack pointer
	// r13          - Relstack pointer pointer
	// rsp+LIBBASE  - LIBBASE
	// rsp+FUNCVEC  - function address
	// rsp+R13STORE - old r13 value
	// rsp          - Return address
aros_thunk_common:
	addq	$24,(%r13)		// Adjust relstack pointer
	movq	LIBBASE(%rsp),%r11	// Set new libbase
	movq	%r11,24(%r10)		

	movq	R13STORE(%rsp),%r11	// Save old r13 off-stack
	movq	%r11,8(%r10)		
	popq	%r11			// Save return address off-stack
	movq	%r11,16(%r10)

	// !! Retrurn address popped from stack -> -8 offset
	movq	FUNCVEC-8(%rsp),%r11
	call	*(%r11)			// Call routine

	// NOTE: Don't clobber %rdx - it can be a return value also
	subq	$24,(%r13)		// Adjust relstack pointer
	movq	(%r13),%r10		// %r10 is now relstack pointer again
	movq	16(%r10),%r11		// %r11 is the old return address
	movq	8(%r10),%r13		// Restore r13
	jmp	*%r11			// Return

	.globl	aros_thunk_rellibfunc

	// %rsp(+12..) - Arguments to called function
	// %rsp@(0) - Return address
	// r10 - Offset from current libbase to new libbase
	// r11 - LVO index from new libbase

aros_thunk_rellibfunc:
	// Save old r13
	movq	%r13,R13STORE(%rsp)
	// Save offset
	movq	%r10,TMP(%rsp)
	// Already set LVO offset to FUNCVEC
	movq	%r11,FUNCVEC(%rsp)

	// Get task's tc_SPLower
	movq	SysBase(%rip), %r13
	movq	ThisTask(%r13),%r13
	movq	tc_SPLower(%r13),%r13

	// Retrieve current libbase in r11
	movq	(%r13),%r10
	movq	(%r10),%r11
	
	// Calculate new sysbase and LVO function to call
	addq	TMP(%rsp),%r11		// New libbase pointer
	movq    (%r11),%r11		// New libbase
	movq	%r11,LIBBASE(%rsp)
	addq	%r11,FUNCVEC(%rsp)
		
	// r10          - Relstack pointer
	// r13          - Relstack pointer pointer
	// rsp+LIBBASE  - LIBBASE
	// rsp+FUNCVEC  - function address
	// rsp+R13STORE - old r13 value
	// rsp          - Return address
	jmp	aros_thunk_common
