/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: ANSI C function setjmp(), AArch64 version.
    Lang: english
*/

	#include "aros/aarch64/asm.h"

	/*
	 * struct __jmp_buf { unsigned long retaddr; unsigned long regs[31]; }.
	 * Save the AAPCS64 callee-saved state: x19-x28, fp(x29), lr(x30), sp and
	 * the NEON callee-saved registers d8-d15. retaddr (offset 0) holds the
	 * return address so longjmp() resumes at the setjmp() call site.
	 */
	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(setjmp)
	_FUNCTION(AROS_CDEFNAME(setjmp))

AROS_CDEFNAME(setjmp):
	stp	x19, x20, [x0, #8]
	stp	x21, x22, [x0, #24]
	stp	x23, x24, [x0, #40]
	stp	x25, x26, [x0, #56]
	stp	x27, x28, [x0, #72]
	stp	x29, x30, [x0, #88]
	mov	x1, sp
	str	x1, [x0, #104]
	stp	d8,  d9,  [x0, #112]
	stp	d10, d11, [x0, #128]
	stp	d12, d13, [x0, #144]
	stp	d14, d15, [x0, #160]
	str	x30, [x0, #0]		/* retaddr */
	mov	w0, #0
	ret
