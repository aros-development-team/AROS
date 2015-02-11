/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function siglongjmp()
    Lang: english
*/

#include "aros/arm/asm.h"

	.text
	.align	2
	.global	AROS_CDEFNAME(siglongjmp)
	.type	AROS_CDEFNAME(siglongjmp),%function

AROS_CDEFNAME(siglongjmp):
	mov	ip, r0						/* env into ip register */
	ldr	lr, [ip], #4					/* restore return address */
	movs	r0, r1						/* return value from siglongjmp into r0 and generate condition code */
	moveq	r0, #1						/* if retval = 0, then retval = 1 */
	ldmia	ip!, {r4, r5, r6, r7, r8, r9, sl, fp, sp}	/* restore non-scratch regs */
#ifdef __SOFTFP__
	ldr	r1, 1f
	ldr	r1, [r1]
	mov	r2, AttnFlags
	ldrh	r1, [r1, r2]
	tst	r1, AFF_FPU
	bxeq	lr
#endif
	fldmiax ip!, {d8-d15}					/* Restore VFP registers - we assume they are available! */
	ldr     r1, [ip], #4					/* restore VFP status reg */
  	fmxr    fpscr, r1
	bx      lr						/* Done! */
#ifdef __SOFTFP__
1:	.word	SysBase
#endif
