/*
 * setjmp.s
 *
 *  Created on: Aug 5, 2009
 *      Author: misc
 *  $Id$
 */

/* divide by zero error: either return 0:0 pair or raise exception */
	.align 2
	.weak __aeabi_ldiv0
	.type __aeabi_ldiv0,%function
__aeabi_ldiv0:
	mov		r0,#0
	mov		r1,#0
	mov		r2,#0
	mov		r3,#0
	bx 		lr
