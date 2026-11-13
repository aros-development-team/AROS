/*
 * vfork.s
 *
 *  Created on: Aug 5, 2009
 *      Author: misc
 *
 * $Id$
 */

#include "aros/armeb/asm.h"

	.text
	.align	2
	.global AROS_CDEFNAME(vfork)
	.type	AROS_CDEFNAME(vfork),%function

AROS_CDEFNAME(vfork):
	str	lr, [sp, #-4]!			/* Store link register */
	sub	sp, sp, jmpbuf_SIZEOF		/* Create space for env structure */
	mov	r0, sp
	bl	setjmp				/* Prepare setjmp */

	ldr	r0, [sp, jmpbuf_SIZEOF]		/* restore link register */
	str	r0, [sp, retaddr]		/* save lr as first argument of env structure */

	add	r0, sp, jmpbuf_SIZEOF + 4	/* save previous stack pointer into env structure */
	str	r0, [sp, #9*4]

	mov	r0, sp				/* Argument to vfork() */
	b	__vfork				/* never return... */
