/*
     (C) 1995-96 AROS - The Amiga Research OS
     $Id$
 
     Desc:
     Lang:
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH1(ULONG, Supervisor,
 
    SYNOPSIS
 	AROS_LHA(ULONG_FUNC, userFunction, A5),
 
    LOCATION
 	struct ExecBase *, SysBase, 5, Exec)
 
    FUNCTION
 	Call a routine in supervisor mode. This routine runs on the
 	supervisor stack and must end with a "rte". No registers are spilled,
 	i.e. Supervisor() effectively works like a function call.
 
    INPUTS
 	userFunction - address of the function to be called.
 
    RESULT
 	whatever the function left in the registers
 
    NOTES
 	This function is CPU dependant.
 
    EXAMPLE
 
    BUGS
 	Context switches that happen during the duration of this call are lost.
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Supervisor,Exec)
	.type	AROS_SLIB_ENTRY(Supervisor,Exec),@function
AROS_SLIB_ENTRY(Supervisor,Exec):

	mfmsr	r0
 	/* No trap? Then this was called from supervisor mode */
 	ljmp	arg0

