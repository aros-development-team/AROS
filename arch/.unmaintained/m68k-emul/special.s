/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Default trap handler
    Lang: english
*/

	#include "machine.i"

	/* Never Called */
	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(TrapHandler,Exec)
	.type	AROS_SLIB_ENTRY(TrapHandler,Exec),@function
AROS_SLIB_ENTRY(TrapHandler,Exec):
	rts
