|*****************************************************************************
|
|   NAME
|
|	__AROS_LH3(APTR, CachePreDMA,
|
|   SYNOPSIS
|	__AROS_LA(APTR,    address, A0),
|	__AROS_LA(ULONG *, length,  A1),
|	__AROS_LA(ULONG,   flags,  D0),
|
|   LOCATION
|	struct ExecBase *, SysBase, 127, Exec)
|
|   FUNCTION
|	Do everything necessary to make CPU caches aware that a DMA will happen.
|	Virtual memory systems will make it possible that your memory isn't at
|	one block and not at the address you thought. This function gives you
|	all the information you need to split the DMA request up and to convert
|	virtual to physical addresses.
|
|   INPUTS
|	address - Virtual address of memory affected by the DMA
|	*length - Number of bytes affected
|	flags	- DMA_Continue	  - This is a call to continue a request that
|				    was broken up.
|		  DMA_ReadFromRAM - Indicate that the DMA goes from RAM
|				    to the device. Set this bit in bot calls.
|
|   RESULT
|	The physical address in memory.
|	*length contains the number of contiguous bytes in physical memory.
|
|   NOTES
|	DMA must follow a call to CachePreDMA() and must be followed
|	by a call to CachePostDMA().
|
|   EXAMPLE
|
|   BUGS
|
|   SEE ALSO
|	CachePostDMA()
|
|   INTERNALS
|
|   HISTORY
|
|******************************************************************************

	| Simple 68000s have no chaches
	.globl	_Exec_CachePreDMA
_Exec_CachePreDMA:
	rts

