/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH3(void, CacheClearE,
 
    SYNOPSIS
 	AROS_LHA(APTR,  address, A0),
 	AROS_LHA(ULONG, length,  D0),
 	AROS_LHA(ULONG, caches,  D1),
 
    LOCATION
 	struct ExecBase *, SysBase, 107, Exec)
 
    FUNCTION
 	Flushes the contents of the CPU chaches for a given area of memory
 	and a given cache.
 
    INPUTS
 	address - First byte in memory
 	length	- number of bytes (~0 means all addresses).
 	caches	- CPU caches affected
 		    CACRF_ClearI - Instruction cache
 		    CACRF_ClearD - Data cache
 
    RESULT
 
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
	.globl	AROS_SLIB_ENTRY(CacheClearE,Exec)
	.type	AROS_SLIB_ENTRY(CacheClearE,Exec),@function
AROS_SLIB_ENTRY(CacheClearE,Exec):
	/* Simple 68000s have no caches */
	rts

