/*
 * vfork_longjmp.s
 *
 *  Created on: Aug 5, 2009
 *      Author: misc
 *
 * $Id$
 */

#include "aros/arm/asm.h"

	.text
	.align	2
	.global	AROS_CDEFNAME(vfork_longjmp)
	.type	AROS_CDEFNAME(vfork_longjmp),%function

AROS_CDEFNAME(vfork_longjmp):
	mov	ip, r0						/* env into ip register */
	ldr	lr, [ip], #4					/* restore return address */
	mov	r0, r1						/* return value from longjmp into r0 and generate condition code */
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
	bx	lr						/* Done! */
#ifdef __SOFTFP__
1:	.word	SysBase
#endif
