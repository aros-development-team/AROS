/*
 * longjmp.s
 *
 *  Created on: Aug 5, 2009
 *      Author: misc
 *  $Id$
 */

#include "aros/arm/asm.h"

	.text
	.align	2
	.global	AROS_CDEFNAME(longjmp)
	.type	AROS_CDEFNAME(longjmp),%function

AROS_CDEFNAME(longjmp):
	mov	ip, r0						/* env into ip register */
	ldr	lr, [ip], #4					/* restore return address */
	movs	r0, r1						/* return value from longjmp into r0 and generate condition code */
	moveq	r0, #1						/* if retval = 0, then retval = 1 */
	ldmia	ip!, {r4, r5, r6, r7, r8, r9, sl, fp, sp}	/* restore non-scratch regs */
	fldmiad ip!, {d8-d15}					/* Restore VFP registers - we assume they are available! */
	ldr     r1, [ip], #4					/* restore VFP status reg */
  	fmxr    fpscr, r1
	bx      lr						/* Done! */
