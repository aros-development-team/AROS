/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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
