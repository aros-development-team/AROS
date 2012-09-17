/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheClearE() - Clear the caches with extended control.
    Lang: english
*/

#include <aros/config.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

#include "kernel_syscall.h"

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

    char *start = (char*)((IPTR)address & 0xffffffe0);
    char *end = (char*)(((IPTR)address + length + 31) & 0xffffffe0);
    char *ptr;
    
    /* Flush data caches and mark cache lines invalid */
    if (caches & CACRF_ClearD)
    {
        for (ptr = start; ptr < end; ptr +=32)
        {
            asm volatile("dcbf 0,%0"::"r"(ptr));
        }
        asm volatile("sync");
    }

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
    if (caches & CACRF_InvalidateD)
    {
        register APTR addr asm ("r4") = address;
        register ULONG len asm ("r5") = length;
        asm volatile("li %%r3,%0; sc"::"i"(SC_INVALIDATED),"r"(addr),"r"(len):"memory","r3");
    }
#endif

    if (caches & CACRF_ClearI) /* Clear ICache with DCache together */
    {
        for (ptr = start; ptr < end; ptr +=32)
        {
            asm volatile("icbi 0,%0"::"r"(ptr));
        }
    
        asm volatile("sync; isync; ");
    }

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
