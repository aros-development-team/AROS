/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

	#include "machine.i"

	/* Never Called */
	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(TrapHandler,Exec)
	.type	AROS_SLIB_ENTRY(TrapHandler,Exec),@function
AROS_SLIB_ENTRY(TrapHandler,Exec):
	rts
