/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: setsr.S 14879 2002-07-01 03:55:14Z bergers $
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH1(ULONG, SetSR,
 
    SYNOPSIS
        AROS_LHA(ULONG, new , D0),
        AROS_LHA(ULONG, mask, D1),
 
    LOCATION
 	struct ExecBase *, SysBase, 24, Exec)
 
    FUNCTION
 
    INPUTS

    RESULT
 
    NOTES
 	This function is CPU dependant.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "../include/aros/machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(SetSR,Exec)

AROS_SLIB_ENTRY(SetSR,Exec):

	/*
	 * Switch to Supervisor Mode by calling a trap function
	 */
#ifndef DoRegisterCalls
	move.l	8(%sp),%d1
	move.l	4(%sp),%d0
#endif
	move.l  %d7,-(%sp)
	move.l  #4,%d7     // #4 => call _sys_trap1_SetSR
	trap    #1
	move.l  (%sp)+,%d7 // d0 holds old value of SR.
	rts

	.globl _sys_trap1_SetSR
_sys_trap1_SetSR:
	/*
	 * Stack layout in this function
	 * D2  0(%sp)
	 * SR  4(%sp)  from trap call
	 * PC  6(%sp)
	 */
	move.l	%d2,-(%sp)
	and.w	%d1,%d0     // isolate the flags to be changed
	not.w	%d1         // invert the mask
	move.w	4(%sp),%d2  // target for sr is register d2
	and.w	%d1,%d2     // isolate the flags to remain untouched
	or.w	%d0,%d2     // generate new sr
	move.w	4(%sp),%d0  // old value of sr goes into d0
	move.w	%d2,4(%sp)  // save the modified target
	not.w	%d1         // revert the change
	move.l	(%sp)+,%d2
	rte
