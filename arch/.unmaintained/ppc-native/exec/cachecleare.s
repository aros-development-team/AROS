/*
     (C) 1995-96 AROS - The Amiga Research OS
     $Id$
 
     Desc:
     Lang:
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
	Clears the PPC cache block to which address parameter belongs
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheClearE,Exec)
	.type	AROS_SLIB_ENTRY(CacheClearE,Exec),@function
AROS_SLIB_ENTRY(CacheClearE,Exec):
	PROLOG
	CMPWI	R4,CACRF_ClearI		/* clear IC? */
	BNE	data			/* no? so maybe data? */
	ICBI	0,R13			/* yes? flush IC */
	EPILOG
data:
	CMPWI R4,CACRF_ClearD		/* is it DC? */
	BNE	finish			/* no? so exit! */
	DCBF	0,R13			/* yes? flush DC */
finish:
	EPILOG
