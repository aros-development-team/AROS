/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: vfork_longjmp.s 29500 2008-09-22 11:55:20Z sszymczy $
*/

/* This function works the same as longjmp() except it lacks the argument 
   check. It's used only by vfork() implementation. */

#include "aros/m68k/asm.h"

/*	.text
	_ALIGNMENT
	.globl AROS_CDEFNAME(vfork_longjmp)
	_FUNCTION(vfork_longjmp) */

	.text
	.align 2
	.global AROS_CDEFNAME(vfork_longjmp)
	.type AROS_CDEFNAME(vfork_longjmp),%function


AROS_CDEFNAME(vfork_longjmp):
    #warning "m68k vfork_longjump not implemented"
    rts
    
