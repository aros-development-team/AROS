/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/******************************************************************************

    NAME
        AROS_LH0(void, CacheClearU,

    LOCATION
        struct ExecBase *, SysBase, 106, Exec)

    FUNCTION
	Flushes the contents of all CPU chaches in a simple way.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	Currently this only works on Linux/m68k.
	
    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

	#include "machine.i"

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(CacheClearU,Exec)
	.type	AROS_SLIB_ENTRY(CacheClearU,Exec),@function
AROS_SLIB_ENTRY(CacheClearU,Exec):
	movem.l	%d2-%d4,-(%sp)
	move.l	#123,%d0
	clr.l	%d1
	moveq	#3,%d2
	moveq	#3,%d3
	clr.l	%d4
	trap	#0
	movem.l	(%sp)+,%d2-%d4
	rts

