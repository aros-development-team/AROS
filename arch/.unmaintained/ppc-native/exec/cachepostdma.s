/*
     (C) 1995-96 AROS - The Amiga Research OS
     $Id$
 
     Desc:
     Lang:
*/

/******************************************************************************
 
    NAME
 
 	AROS_LH3(void, CachePostDMA,
 
    SYNOPSIS
 	AROS_LHA(APTR,    address, A0),
 	AROS_LHA(ULONG *, length,  A1),
 	AROS_LHA(ULONG,   flags,  D0),
 
    LOCATION
 	struct ExecBase *, SysBase, 128, Exec)
 
    FUNCTION
 	Do everything necessary to make CPU caches aware that a DMA has
 	happened.
 
    INPUTS
 	address - Virtual address of memory affected by the DMA
 	*length - Number of bytes affected
 	flags	- DMA_NoModify	  - Indicate that the memory did not change.
 		  DMA_ReadFromRAM - Indicate that the DMA goes from RAM
 				    to the device. Set this bit in bot calls.
 
    RESULT
 
    NOTES
 	DMA must follow a call to CachePreDMA() and must be followed
 	by a call to CachePostDMA().
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	CachePreDMA()
 
    INTERNALS
	According to Phase 5 technical documentation implementing this is
	a bit tricky. Until I can fully understand it, we just flush both caches
	instead
 
    HISTORY
 
******************************************************************************/

/*
   XDEF AROS_SLIB_ENTRY(CachePostDMA,Exec)   	; for 68000/10/20
   XDEF AROS_SLIB_ENTRY(CachePostDMA_30,Exec)	; for 68030+
   XDEF AROS_SLIB_ENTRY(CachePostDMA_40,Exec)	; for 68040/68060
*/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CachePostDMA,Exec)
	.type	AROS_SLIB_ENTRY(CachePostDMA,Exec),@function
AROS_SLIB_ENTRY(CachePostDMA,Exec):
	PROLOG
	ICBI	0,R13
	DCBF	0,R13
	EPILOG