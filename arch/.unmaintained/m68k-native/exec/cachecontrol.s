/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH2(ULONG, CacheControl,
 
    SYNOPSIS
 	AROS_LHA(ULONG, cacheBits, D0),
 	AROS_LHA(ULONG, cacheMask, D1),
 
    LOCATION
 	struct ExecBase *, SysBase, 107, Exec)
 
    FUNCTION
 	Change/read the values in the 68030 cacr register. Only the bits set
 	in the mask parameter are affected.
 
    INPUTS
 	cacheBits - new bit values.
 	cacheMask - Bits to change.
 
    RESULT
 	Old contents of cacr register.
 
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
	.globl	AROS_SLIB_ENTRY(CacheControl,Exec)
	.type	AROS_SLIB_ENTRY(CacheControl,Exec),@function
AROS_SLIB_ENTRY(CacheControl,Exec):
	/* Simple 68000s have no chaches */
	rts

