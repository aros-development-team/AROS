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
	subr
	li	ret,0
	rts
