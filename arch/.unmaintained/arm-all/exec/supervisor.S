/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
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
	// userFunction parameter is in r0
	// execbase is in r1
	stmdb	sp!,{r4, lr}
	mov	r4,r0
	swi	#8         // #8 => call software interrupt handler
	ldmdb	sp!,{r4, lr}
	mov	pc, lr

	.globl _sys_trap_Supervisor
_sys_trap_Supervisor:
	strdb	sp!,{lr}
	mov	lr, pc      // should add 4 to lr here?
	mov	pc, r4
	ldmdb	sp!,{lr}
	mov	pc, lr

	