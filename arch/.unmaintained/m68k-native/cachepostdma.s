|******************************************************************************
|
|   NAME
|
|	__AROS_LH3(void, CachePostDMA,
|
|   SYNOPSIS
|	__AROS_LA(APTR,    address, A0),
|	__AROS_LA(ULONG *, length,  A1),
|	__AROS_LA(ULONG,   flags,  D0),
|
|   LOCATION
|	struct ExecBase *, SysBase, 128, Exec)
|
|   FUNCTION
|	Do everything necessary to make CPU caches aware that a DMA has
|	happened.
|
|   INPUTS
|	address - Virtual address of memory affected by the DMA
|	*length - Number of bytes affected
|	flags	- DMA_NoModify	  - Indicate that the memory didn't change.
|		  DMA_ReadFromRAM - Indicate that the DMA goes from RAM
|				    to the device. Set this bit in bot calls.
|
|   RESULT
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
|	CachePreDMA()
|
|   INTERNALS
|
|   HISTORY
|
|******************************************************************************

	| Simple 68000s have no chaches
	.globl	_Exec_CachePostDMA
_Exec_CachePostDMA:
	rts

