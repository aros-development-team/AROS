/*
     (C) 1995-96 AROS - The Amiga Replacement OS
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
 	happened. I'm not sure whether it's important under PC or not.
	I guess that caches are automaticaly updated while DMA transfers,
	but it will be OK if we flush all caches.
 
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
 
    HISTORY
 
******************************************************************************/

#include "machine.i"

	.text
	.globl	AROS_SLIB_ENTRY(CachePostDMA,Exec)
	.type	AROS_SLIB_ENTRY(CachePostDMA,Exec),@function
AROS_SLIB_ENTRY(CachePostDMA,Exec):
	wbinvd
	ret
