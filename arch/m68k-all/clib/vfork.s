/*
 * vfork.s
 *
 *  Created on: Aug 23, 2010
 *      Author: misc
 *
 * $Id$
 */

#include "aros/m68k/asm.h"

	.text
	.align 2
	.global AROS_CDEFNAME(vfork)
	.type AROS_CDEFNAME(vfork),%function

AROS_CDEFNAME(vfork):
    #warning "m68k vfork not implemented"
    rts

