/*
     (C) 1995-96 AROS - The Amiga Replacement OS
     $Id$
 
     Desc:
     Lang:
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
	This procedure will allow to turn cache on/off on i386+ machines.
 
    INPUTS
 	cacheBits - new bit values.
 	cacheMask - Bits to change.
 
    RESULT
 	Old contents of cacr (not under intel!) register.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.globl	AROS_SLIB_ENTRY(CacheControl,Exec)
	.type	AROS_SLIB_ENTRY(CacheControl,Exec),@function
AROS_SLIB_ENTRY(CacheControl,Exec):
	movl	$0,%eax
	ret

