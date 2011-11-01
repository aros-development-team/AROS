/*
 * setjmp.s
 *
 *  Created on: Aug 5, 2009
 *      Author: misc
 *  $Id$
 */

#include "aros/arm/asm.h"

	.text
	.align	2
	.global AROS_CDEFNAME(setjmp)
	.type	AROS_CDEFNAME(setjmp),%function

AROS_CDEFNAME(setjmp):
	mov	ip, r0						/* Get the env address */
	str	lr, [ip], #4					/* store return address explicitly */
	stmia	ip!, {r4, r5, r6, r7, r8, r9, sl, fp, sp}	/* store non-scratch regs */
	fstd	d8, [ip]					/* Store VFP registers - we assume they are available! */
	fstd	d9, [ip, #8]
	fstd	d10, [ip, #16]
	fstd	d11, [ip, #24]
	fstd	d12, [ip, #32]
	fstd	d13, [ip, #40]
	fstd	d14, [ip, #48]
	fstd	d15, [ip, #56]
	fmrx	r2, fpscr					/* VFP condition codes */
	str	r2, [ip, #64]
	mov	r0, #0						/* return zero */
	bx	lr
