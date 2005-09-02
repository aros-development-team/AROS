/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: cachecleare.c 21699 2004-06-10 10:51:27Z weissms $

    Desc: CacheClearE() - Clear the caches with extended control.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH3(void, CacheClearE,

/*  SYNOPSIS */
	AROS_LHA(APTR, address, A0),
	AROS_LHA(ULONG, length, D0),
	AROS_LHA(ULONG, caches, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 107, Exec)

/*  FUNCTION
	Flush the contents of the CPU instruction or data caches. If some
	of the cache contains dirty data, push it to memory first.

	For most systems DMA will not effect processor caches. If *any*
	external (non-processor) event changes system memory, you MUST
	clear the cache. For example:

	    DMA
	    Code relocation to run at a different address
	    Building jump tables
	    Loading code from disk

    INPUTS
	address -   Address to start the operation. This address may be
		    rounded DOWN due to hardware granularity.
	length	-   Length of the memory to flush. This will be rounded
		    up, of $FFFFFFFF to indicate that all addresses
		    should be cleared.
	caches	-   Bit flags to indicate which caches should be cleared

			CACRF_ClearI	-   Clear the instruction cache
			CACRF_ClearD	-   Clear the data cache

		    All other bits are reserved.

    RESULT
	The caches will be flushed.

    NOTES
	It is possible that on some systems the entire cache will be
	even if this was not the specific request.

    EXAMPLE

    BUGS

    SEE ALSO
	CacheClearU(), CacheControl()

    INTERNALS
	This is a rather CPU dependant function. You should replace it
	in your $(KERNEL).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(FLUSH_CACHES)
    {
	ULONG blocks;

	// cache block alignment
	ULONG align = (ULONG) address & ~(CACHE_BLOCK_SIZE - 1);
	
	// round up
	length = length + CACHE_BLOCK_SIZE - 1;
	blocks = length / CACHE_BLOCK_SIZE;
	if (caches & CACRF_ClearD)
	{
	    ULONG i;
	    for (i = 0; i < blocks; i++)
		DATA_CACHE_BST(align + i * CACHE_BLOCK_SIZE);
	    
	    SYNC;
	}

	if (caches & CACRF_ClearI)
	{
	    ULONG i;
	    for (i = 0; i < blocks; i++)
		INSTR_CACHE_BINV(align + i * CACHE_BLOCK_SIZE);
	    
	    // SYNC for G4
	    SYNC;
	    ISYNC;
	}
    }
#else
    aros_print_not_implemented("CacheClearE");
#endif /* defined(FLUSH_CACHES) */

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
