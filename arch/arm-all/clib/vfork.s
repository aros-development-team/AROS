/*
 * vfork.s
 *
 *  Created on: Aug 5, 2009
 *      Author: misc
 *
 * $Id:$
 */

#include "aros/arm/asm.h"

	.text
	.align 2
	.global AROS_CDEFNAME(vfork)
	.type AROS_CDEFNAME(vfork),%function

AROS_CDEFNAME(vfork):
	str		lr, [sp, #-4]!		/* Store link register */
	sub		sp, sp, #260		/* Create space for env structure */
	mov		r0, sp
	bl		setjmp				/* Prepare setjmp */

	ldr		r0, [sp, #264]		/* restore link register */
	str		r0, [sp, #0*4]		/* save lr as first argument of env structure */

	add		r0, sp, #264		/* save previous stack pointer into env structure */
	str		r0, [sp, #9*4]

	b		__vfork				/* never return... */

