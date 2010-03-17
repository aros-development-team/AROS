/*
 * vfork.s
 *
 *  Created on: Aug 23, 2010
 *      Author: misc
 *
 * $Id: vfork.s 31678 2009-08-05 21:31:08Z schulz $
 */

#include "aros/m68k/asm.h"

	.text
	.align 2
	.global AROS_CDEFNAME(vfork)
	.type AROS_CDEFNAME(vfork),%function

AROS_CDEFNAME(vfork):
    #warning "m68k vfork not implemented"
    rts

