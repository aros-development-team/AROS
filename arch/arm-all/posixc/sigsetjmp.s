/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function sigsetjmp()
    Lang: english
*/

#include "aros/arm/asm.h"

	.text
	.align	2
	.global AROS_CDEFNAME(sigsetjmp)
	.type	AROS_CDEFNAME(sigsetjmp),%function

AROS_CDEFNAME(sigsetjmp):
	mov	ip, r0						/* Get the env address */
	str	lr, [ip], #4					/* store return address explicitly */
	stmia	ip!, {r4, r5, r6, r7, r8, r9, sl, fp, sp}	/* store non-scratch regs */
	mov	r0, #0						/* return zero */
#ifdef __SOFTFP__
	ldr	r1, 1f
	ldr	r1, [r1]
	mov	r2, AttnFlags
	ldrh	r1, [r1, r2]
	tst	r1, AFF_FPU
	bxeq	lr
#endif
	fstmiax	ip!, {d8-d15}					/* Store VFP registers - we assume they are available! */
	fmrx	r2, fpscr					/* VFP condition codes */
	str	r2, [ip], #4
	bx	lr
#ifdef __SOFTFP__
1:	.word	SysBase
#endif
