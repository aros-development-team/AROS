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

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(UserState,Exec)
	.type	AROS_SLIB_ENTRY(UserState,Exec),@function
AROS_SLIB_ENTRY(UserState,Exec):
	/* simply return if argument is NULL */
	push	scr
	mflr	scr
	push	scr
	cmpdi	arg0,0
	bne	nonzero
	pop	scr
	mtlr	scr
	pop	scr
nonzero:
	pop	scr
	mtsrr0	scr
	pop	scr
	rfi
