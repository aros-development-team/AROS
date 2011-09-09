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
	fldmiax ip!, {d8-d15}					/* Restore VFP registers - we assume they are available! */
	ldr     r1, [ip], #4					/* restore VFP status reg */
  	fmxr    fpscr, r1
  	ldr	r1, [ip], #4					/* restore *(SysBase->ThisTask->tc_SPLower) */
        ldr	r2, 1f
        ldr	r2, [r2]
        ldr	r2, [r2, ThisTask]
        ldr	r2, [r2, tc_SPLower]
        str	r1, [r2]
	bx	lr						/* Done! */
1:	.word	SysBase
