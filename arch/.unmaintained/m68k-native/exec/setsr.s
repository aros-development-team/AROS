/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH2(ULONG, SetSR,
 
    SYNOPSIS
 	AROS_LHA(ULONG, newSR, D0),
 	AROS_LHA(ULONG, mask,  D1),
 
    LOCATION
 	struct ExecBase *, SysBase, 24, Exec)
 
    FUNCTION
 	Read/Modify the CPU status register in an easy way. Only the bits set in
 	the mask parameter will be changed.
 
    INPUTS
 	newSR - New contents of sr.
 	mask  - Bits to change.
 
    RESULT
 	Old contents of sr.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(SetSR,Exec)
	.type	AROS_SLIB_ENTRY(SetSR,Exec),@function
AROS_SLIB_ENTRY(SetSR,Exec):
	/* Do the real work in supervisor mode
	   Preserve a5 in a0 (faster than stack space)
	*/
	move.l	a5,a0
	lea.l	setsrsup,a5
	jsr	Supervisor(a6)
	move.l	a0,a5
	rts

setsrsup:
	/* The old value of sr now lies on the top of the stack.
	   d1 = (mask & newSR) | (~mask & SR)
	*/
	and.w	d1,d0
	eor.w	#-1,d1
	and.w	(sp),d1
	or.w	d0,d1

	/* Get returncode */
	clr.l	d0
	move.w	(sp),d0

	/* Set new sr value */
	move.w	d1,(sp)
	rte

