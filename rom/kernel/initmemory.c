#include <aros/config.h>
#include <aros/kernel.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include "memory_intern.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1(void, KrnInitMemory,

/*  SYNOPSIS */
	AROS_LHA(struct MemHeader *, mh, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 31, Kernel)

/*  FUNCTION
	Initialize kernel memory management on a given memory region

    INPUTS
    	mh - Address of a filled in structure describing the region.

    RESULT
    	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if USE_MMU
    struct BlockHeader *head;
    IPTR align;
    APTR end;
    IPTR memsize;
    IPTR mapsize;
    IPTR p;
    UBYTE free;

    if (!KernelBase->kb_PageSize)
	return;
    
    head = (struct BlockHeader *)mh->mh_First;
    align = KernelBase->kb_PageSize - 1;
    
    /* Fill in legacy MemChunk structure */
    head->mc.mc_Next = NULL;
    head->mc.mc_Bytes = 0;

    InitSemaphore(&head->sem);

    /*
     * Page-align boundaries. 
     * We intentionally make it start pointing to the previous page,
     * we'll jump to the next page later, in the loop.
     */
    head->start = (APTR)((IPTR)head->map & ~align);
    end = (APTR)(((IPTR)mh->mh_Upper + 1) & ~align);

    do
    {
    	/* Skip one page. This reserves some space (one page or less) for allocations map. */
    	head->start += KernelBase->kb_PageSize;
    	/* Calculate resulting map size */
	mapsize = (head->start - (APTR)head->map) / sizeof(ULONG);
	/* Calculate number of free bytes and pages */
	memsize = end - head->start;
    	head->size = memsize / KernelBase->kb_PageSize;
	/*
	 * Repeat the operation if there's not enough memory for allocations map.
	 * This will take one more page from the area and use it for the map.
	 */
    } while (mapsize < head->size);

    /* Mark all pages as free */
    p = head->size;
    free = 1;
    do {
    	head->map[--p] = free;
	if (free < 127)
	    free++;
    } while (p > 0);

    /* Set free space counter */
    mh->mh_Free = memsize;

    /* Disable access to unallocated pages */
    KrnSetProtection(head->start, memsize, 0);

#endif

    AROS_LIBFUNC_EXIT
}
