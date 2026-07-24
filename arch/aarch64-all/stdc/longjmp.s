/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: ANSI C function longjmp(), AArch64 version.
    Lang: english
*/

	#include "aros/aarch64/asm.h"

	/*
	 * longjmp(jmp_buf env, int val): restore the callee-saved state saved by
	 * setjmp() and return to its call site with the value val (or 1 if val is
	 * 0, as required by the C standard).
	 */
	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(longjmp)
	_FUNCTION(AROS_CDEFNAME(longjmp))

AROS_CDEFNAME(longjmp):
	ldp	x19, x20, [x0, #8]
	ldp	x21, x22, [x0, #24]
	ldp	x23, x24, [x0, #40]
	ldp	x25, x26, [x0, #56]
	ldp	x27, x28, [x0, #72]
	ldp	x29, x30, [x0, #88]
	/*
	 * Return via the retaddr slot rather than the x30 copy at offset 96:
	 * for a plain setjmp the two are identical, but the vfork machinery
	 * (posixc __vfork) patches retaddr to redirect the jump.
	 */
	ldr	x30, [x0, #retaddr]
	ldr	x2, [x0, #104]
	mov	sp, x2
	ldp	d8,  d9,  [x0, #112]
	ldp	d10, d11, [x0, #128]
	ldp	d12, d13, [x0, #144]
	ldp	d14, d15, [x0, #160]
	cmp	w1, #0
	mov	w0, w1
	cinc	w0, w0, eq		/* val==0 -> return 1 */
	ret
