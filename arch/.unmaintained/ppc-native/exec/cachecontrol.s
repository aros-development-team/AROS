/*
     (C) 1995-96 AROS - The Amiga Research OS
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
	It does nothing on the PPC
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheControl,Exec)
	.type	AROS_SLIB_ENTRY(CacheControl,Exec),@function
AROS_SLIB_ENTRY(CacheControl,Exec):
	PROLOG
	LI	R3,0	/* return 0 */
	EPILOG
