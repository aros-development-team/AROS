/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH1(void, UserState,
 
    SYNOPSIS
 	AROS_LHA(APTR, sysStack, D0),
 
    LOCATION
 	struct ExecBase *, SysBase, 26, Exec)
 
    FUNCTION
 	Return to user mode after a call to SuperState().
 
    INPUTS
 	sysStack - The returncode from SuperState().
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	SuperState(), Supervisor()
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(UserState,Exec,26)
	.type	AROS_SLIB_ENTRY(UserState,Exec,26),@function
AROS_SLIB_ENTRY(UserState,Exec,26):
	/* simply return if argument is NULL */
	tst.l	%d0
	bne	nonzero
	rts
nonzero:
	/* Transfer sp */
	move.l	%sp,%usp

	/* Set old supervisor sp */
	move.l	%d0,%sp

	/* And return. This jumps directly to a rts. */
	rte

