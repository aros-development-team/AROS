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
	fstmiax	ip!, {d8-d15}					/* Store VFP registers - we assume they are available! */
	fmrx	r2, fpscr					/* VFP condition codes */
	str	r2, [ip], #4
	mov	r0, #0						/* return zero */
	bx	lr
