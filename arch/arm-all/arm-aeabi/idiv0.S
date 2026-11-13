/*
 * setjmp.s
 *
 *  Created on: Aug 5, 2009
 *      Author: misc
 *  $Id$
 */

/* divide by zero error: either return 0:0 pair or raise exception */
	.align 2
	.weak __aeabi_idiv0
	.type __aeabi_idiv0,%function
__aeabi_idiv0:
	mov		r0,#0
	mov		r1,#0
	bx 		lr
