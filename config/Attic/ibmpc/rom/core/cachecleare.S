/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: CacheClearE() - Clear the caches with extended control.
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_LH3(void, CacheClearE,

    SYNOPSIS
	AROS_LHA(APTR, address, A0),
	AROS_LHA(ULONG, length, D0),
	AROS_LHA(ULONG, caches, D1),

    LOCATION
	struct ExecBase *, SysBase, 107, Exec)

    FUNCTION
	Flush the contents of the CPU instruction or data caches. If some
	of the cache contains dirty data, push it to memory first.

	For most systems DMA will not effect processor caches. If *any*
	external (non-processor) event changes system memory, you MUST
	clear the cache. For example:

	    DMA
	    Code relocation to run at a different address
	    Building jump tables
	    Loading code from disk

	On PC all caches have special bus which catches DMA accesses. Caches
	are replaced automatically (I hope ;-))

    INPUTS
	address -   Address to start the operation. This address may be
		    rounded DOWN due to hardware granularity.
	length  -   Length of the memory to flush. This will be rounded
		    up, of $FFFFFFFF to indicate that all addresses
		    should be cleared.
	caches  -   Bit flags to indicate which caches should be cleared

			CACRF_ClearI    -   Clear the instruction cache
			CACRF_ClearD    -   Clear the data cache

		    All other bits are reserved.

    RESULT
	The caches will be flushed.

    NOTES
	It is possible that on some systems the entire cache will be
	even if this was not the specific request.

    EXAMPLE

    BUGS

    SEE ALSO
	CacheClearE(), CacheControl()

    HISTORY

******************************************************************************/

#include "machine.i"

	    .text
	    .globl	AROS_SLIB_ENTRY(CacheClearE,Exec)
	    .type	AROS_SLIB_ENTRY(CacheClearE,Exec),@function
AROS_SLIB_ENTRY(CacheClearE,Exec):
	    wbinvd
	    ret
