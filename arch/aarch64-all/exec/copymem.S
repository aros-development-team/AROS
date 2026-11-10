/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CopyMem() - NEON copy, AArch64 version.

    Unaligned accesses are fine on Normal memory (SCTLR_EL1.A is clear).
    Callers must not overlap source and dest (see the CopyMem() autodoc),
    so the head and tail stores may cover the same bytes twice and the
    copy runs forward.
*/

	#include <aros/aarch64/asm.h>

	.text
	_ALIGNMENT
	.globl	AROS_SLIB_ENTRY(CopyMem,Exec,104)
	_FUNCTION(AROS_SLIB_ENTRY(CopyMem,Exec,104))

AROS_SLIB_ENTRY(CopyMem,Exec,104):

	// x0 = source, x1 = dest, x2 = size
	add	x4, x0, x2		// source end
	add	x5, x1, x2		// dest end
	cmp	x2, #16
	b.lo	.Lsmall
	cmp	x2, #32
	b.hi	.L33_64

	ldr	q0, [x0]
	ldr	q1, [x4, #-16]
	str	q0, [x1]
	str	q1, [x5, #-16]
	ret

.L33_64:
	cmp	x2, #64
	b.hi	.L65_128
	ldp	q0, q1, [x0]
	ldp	q2, q3, [x4, #-32]
	stp	q0, q1, [x1]
	stp	q2, q3, [x5, #-32]
	ret

.L65_128:
	cmp	x2, #128
	b.hi	.Llarge
	ldp	q0, q1, [x0]
	ldp	q2, q3, [x0, #32]
	ldp	q4, q5, [x4, #-64]
	ldp	q6, q7, [x4, #-32]
	stp	q0, q1, [x1]
	stp	q2, q3, [x1, #32]
	stp	q4, q5, [x5, #-64]
	stp	q6, q7, [x5, #-32]
	ret

.Lsmall:
	tbz	x2, #3, 1f		// 8..15
	ldr	x6, [x0]
	ldr	x7, [x4, #-8]
	str	x6, [x1]
	str	x7, [x5, #-8]
	ret
1:	tbz	x2, #2, 2f		// 4..7
	ldr	w6, [x0]
	ldr	w7, [x4, #-4]
	str	w6, [x1]
	str	w7, [x5, #-4]
	ret
2:	cbz	x2, 3f			// 1..3
	lsr	x8, x2, #1
	ldrb	w6, [x0]
	ldrb	w7, [x0, x8]
	ldrb	w9, [x4, #-1]
	strb	w6, [x1]
	strb	w7, [x1, x8]
	strb	w9, [x5, #-1]
3:	ret

.Llarge:
	// Head, tail and first block are loaded before the first store.
	ldp	q4, q5, [x4, #-64]
	ldp	q6, q7, [x4, #-32]
	ldr	q16, [x0]

	// Bias both pointers so the dest stores are 16-byte aligned.
	and	x6, x1, #15
	sub	x0, x0, x6
	sub	x3, x1, x6
	add	x2, x2, x6

	ldp	q0, q1, [x0, #16]
	ldp	q2, q3, [x0, #48]
	str	q16, [x1]

	// x2 = bytes left between this block and the tail
	subs	x2, x2, #144
	b.ls	2f
1:	stp	q0, q1, [x3, #16]
	ldp	q0, q1, [x0, #80]
	stp	q2, q3, [x3, #48]
	ldp	q2, q3, [x0, #112]
	add	x0, x0, #64
	add	x3, x3, #64
	subs	x2, x2, #64
	b.hi	1b
2:	stp	q0, q1, [x3, #16]
	stp	q2, q3, [x3, #48]
	stp	q4, q5, [x5, #-64]
	stp	q6, q7, [x5, #-32]
	ret
