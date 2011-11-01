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
	fldd	d8, [ip]					/* Restore VFP registers - we assume they are available! */
	fldd	d9, [ip, #8]
	fldd	d10, [ip, #16]
	fldd	d11, [ip, #24]
	fldd	d12, [ip, #32]
	fldd	d13, [ip, #40]
	fldd	d14, [ip, #48]
	fldd	d15, [ip, #56]
	ldr     r1, [ip, #64]					/* restore VFP status reg */
  	fmxr    fpscr, r1
	bx      lr						/* Done! */
