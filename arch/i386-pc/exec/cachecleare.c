/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheClearE() - Clear the caches with extended control.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

void Exec_Wbinvd();

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
    Due to the strong cache coherency of x86 systems this function
    is actually not needed. CPU snoops the address lines and 
    invalidate all cache which is out-of-date. It is valid for both
    D and I caches). Even a BM-DMA transfer are perfectly safe here.

    The above statement is not valid (at least) in case of AGP transfers. In
    such case all operations on GATT table need to be followed by FULL CPU
    cache flush or they are not visible by the video card.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* A full flush has been requested */
    if (length == 0xFFFFFFFF)
    {
        Supervisor((ULONG_FUNC)Exec_Wbinvd);
        return;
    }

    /* A partial flush requested */
    if (caches & CACRF_ClearD)
    {
        /* TODO: Detect if CPU supports clflush instruction and use it instead
           of wbinvd to provide more optimized cache flushes */
        Supervisor((ULONG_FUNC)Exec_Wbinvd);
    }

    AROS_LIBFUNC_EXIT
} /* CacheClearE */

