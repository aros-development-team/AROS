/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: supervisor.S 13176 2002-02-09 06:21:26Z bergers $
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
 	This function is CPU dependent.
 
    EXAMPLE
 
    BUGS
 	Context switches that happen during the duration of this call are lost.
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "../include/aros/machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Supervisor,Exec)

AROS_SLIB_ENTRY(Supervisor,Exec):
#ifndef DoRegisterCalls
	move.l  4(%sp),%a5 // Get the userFunction pointer
#endif
	move.l  %d7,-(%sp)
	move.l  #8,%d7     // #8 => call _sys_trap1_Supervisor
	trap    #1
	move.l  (%sp)+,%d7
	rts

	.globl _sys_trap1_Supervisor
_sys_trap1_Supervisor:
	jmp	(%a5)
