/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Page-based memory allocator, linear algorithm.
    Lang: english
*/

#define __KERNEL_NOLIBBASE__

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/arossupport.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_mm.h>
#include "mm_linear.h"

#define D(x)

/*
 * 'Linear' memory page allocator implementation.
 * Goals of this implementation are simplicity and reduced memory overhead.
 *
 * It's a modified version of exec.library allocator, which works with variable-length blocks
 * of pages. Instead of lists, it keeps the information about allocated/free pages in
 * a linear memory map, which is separated from the data itself. It allows to block all access
 * to unallocated pages. When allocating blocks at arbitrary addresses, the memory space is
 * searched for the best matching block. MEMF_REVERSE can be used to specify search direction.
 */

/*
 * Utility function.
 * Change state of block of 'pages' pages starting at 'first' to 'state'.
 * Checks blocks to the left and to the right from our block and merges/splits
 * blocks if necessary, and updates counters.
 */
static void SetBlockState(struct BlockHeader *head, IPTR first, IPTR pages, page_t state)
{
    /* Check state of block next to our region */
    IPTR p = first + pages;
    page_t cnt = 1;

    if (p > head->size)
    	/* If we claim we want to access more pages than our BlockHeader has, this is bad */
    	Alert(AN_BadFreeAddr);
    else if (p < head->size)
    {
        /*
         * If the block next to our region is in the same state as our
    	 * block will have, pick up initial counter value from it. Our block
    	 * will be merged with the next one.
    	 */
	if (P_STATUS(head->map[p]) == state)
	{
	    cnt = P_COUNT(head->map[p]);
	    INC_COUNT(cnt);
	}
    }

    /*
     * Set state of our block. We set state from last to the first page, and every
     * page adds 1 to the counter (until it hits the limit value).
     */
    do
    {
    	head->map[--p] = cnt | state;
	INC_COUNT(cnt);
    } while (p > first);

    /*
     * If our block starts at page 0, there's nothing to check before it.
     * We're done.
     */
    if (p == 0)
    	return;

    /*
     * Preceding block can have either the same state as our new state,
     * or different state.
     * In both cases its counters need updating.
     * - If it has the same state, we keep current counter value, so blocks
     *   get merged.
     * - If it has different state, we restart counter from 1. This means
     *   we are splitting the block.
     */
    if (P_STATUS(head->map[p-1]) != state)
    {
    	cnt = 1;
    	state = P_STATUS(head->map[p-1]);
    }

    /*
     * Update space counter for the block. We keep going until the state changes
     * again. The block will keep its original state.
     */
    do
    {
    	if (P_STATUS(head->map[--p]) != state)
    	    break;

	head->map[p] = cnt | state;
	INC_COUNT(cnt);
    } while (p > 0);
}

/* Allocate 'size' bytes from MemHeader mh */
APTR mm_Allocate(struct MemHeader *mh, IPTR size, ULONG flags)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    APTR addr = NULL;
    IPTR align = head->pageSize - 1;
    IPTR pages;
    IPTR p;
    IPTR candidate, candidate_size;

    D(bug("[mm_Allocate] Request for %u bytes from BlockHeader %p, KernelBase 0x%p\n", size, head, KernelBase));

    /*
     * Safety checks.
     * If mh_First is NULL, it's ROM header. We can't allocate from it.
     */
    if (!head)
	return NULL;
    /*
     * If either mc_Next or mc_Bytes is not zero, this MemHeader is not
     * managed by us. We can't allocate from it.
     */
    if (head->mc.mc_Next || head->mc.mc_Bytes)
	return NULL;

    /* Pad up size and convert it to number of pages */
    size = (size + align) & ~align;
    pages = size / head->pageSize;

    if (KernelBase)
    	ObtainSemaphore(&head->sem);

    /* Start looking up from page zero */
    p = 0;
    candidate = 0;
    candidate_size = -1;

    /*
     * Look up best matching free block.
     * We walk through the whole memory map in order to identify free blocks
     * and get their sizes. We use the best-match criteria in order to avoid
     * excessive memory fragmentation.
     */
    do
    {
        IPTR start = p;		/* Starting page of the block being examined */
	IPTR free = 0;		/* Count of free pages in the block */

	while (P_STATUS(head->map[p]) == P_FREE)
	{
	    UBYTE cnt = P_COUNT(head->map[p]);	/* Get (partial) block length */

	    free += cnt;			/* Add length to total count */
	    p += cnt;				/* Advance past the block    */

	    if (p == head->size)		/* Reached end of this memory chunk ? */
		break;
	    if (p > head->size)			/* Went past the chunk? This must never happen! */
		Alert(AN_MemCorrupt);
	}

	D(bug("[mm_Allocate] Have %u free pages starting from %u\n", free, start));

	/* Does the block fit ? */
	if (free >= pages)
	{
	    if (flags & MEMF_REVERSE)
	    {
		/*
		 * If MEMF_REVERSE is set, we remember new candidate if its size is less
		 * or equal to current one. This is effectively the same as best-match
		 * lookup starting from the end of the region.
		 */
		if (free <= candidate_size)
		{
		    D(bug("[mm_Allocate] Old candidate %u (size %d)\n", candidate, candidate_size));
		    candidate = start;
		    candidate_size = free;
		    D(bug("[mm_Allocate] New candidate %u (size %d)\n", candidate, candidate_size));
		}
	    }
	    else
	    {
		/*
		 * If the found block has smaller size than the
		 * previous candidate, remember it as a new candidate.
		 */
		if (free < candidate_size)
		{
		    D(bug("[mm_Allocate] Old candidate %u (size %d)\n", candidate, candidate_size));
		    candidate = start;
		    candidate_size = free;
		    D(bug("[mm_Allocate] New candidate %u (size %d)\n", candidate, candidate_size));
		}

		/* If found exact match, we can't do better, so stop searching */
		if (free == pages)
		{
		    D(bug("[mm_Allocate] Exact match\n"));
		    break;
		}
	    }
	}

	/*
	 * If we are at the end of memory map, we have nothing
	 * more to look at. We either already have a candidate,
	 * or no
	 */
	if (p == head->size)
	{
	    D(bug("[mm_Allocate] Reached end of chunk\n"));
	    break;
	}

	D(bug("[mm_Allocate] Allocated block starts at %u\n", p));
	/* Skip past the end of the allocated block */
	while (P_STATUS(head->map[p]) == P_ALLOC)
	{
	    p += P_COUNT(head->map[p]);

	    if (p == head->size)
	    {
	    	D(bug("[mm_Allocate] Reached end of chunk\n"));
		break;
	    }
	    if (p > head->size)
		Alert(AN_MemCorrupt);
	}
	D(bug("[mm_Allocate] Skipped up to page %u\n", p));

    } while (p < head->size);

    /* Found block ? */
    if (candidate_size != -1)
    {
	/* Mark the block as allocated */
	D(bug("[mm_Allocate] Allocating %u pages starting from %u\n", pages, candidate));
	SetBlockState(head, candidate, pages, P_ALLOC);

	/* Calculate starting address of the first page */
	addr = head->start + candidate * head->pageSize;
	/* Update free memory counter */
	mh->mh_Free -= size;	
    }

    if (KernelBase)
    	ReleaseSemaphore(&head->sem);

    D(bug("[mm_Allocate] Allocated at address 0x%p\n", addr));
    return addr;
}

/* Allocate 'size' bytes starting at 'addr' from MemHeader mh */
APTR mm_AllocAbs(struct MemHeader *mh, void *addr, IPTR size)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    IPTR align = head->pageSize - 1;
    IPTR pages;
    IPTR start, p;
    void *ret = NULL;

    D(bug("[mm_Allocate] Request for %u bytes from BlockHeader %p\n", size, head));

    /*
     * Safety checks.
     * If mh_First is NULL, it's ROM header. We can't allocate from it.
     */
    if (!head)
	return NULL;
    /*
     * If either mc_Next or mc_Bytes is not zero, this MemHeader is not
     * managed by us. We can't allocate from it.
     */
    if (head->mc.mc_Next || head->mc.mc_Bytes)
	return NULL;

    /* Align starting address */
    addr = (void *)((IPTR)addr & ~align);

    /* Requested address cat hit our administrative area. We can't satisfy such a request */
    if (addr < head->start)
	return NULL;

    /* Pad up size and convert it to number of pages */
    size = (size + align) & ~align;
    pages = size / head->pageSize;

    ObtainSemaphore(&head->sem);

    /* Get start page number */
    start = (addr - head->start) / head->pageSize;

    /* Check if we have enough free pages starting from the first one */
    p = start;
    while (P_STATUS(head->map[p]) == P_FREE)
    {
	p += P_COUNT(head->map[p]);		/* Advance past the block    */
	if (p >= start + pages)			/* Counted enough free pages? */
	{
	    /* Allocate the block and exit */
	    ret = addr;
	    SetBlockState(head, start, pages, P_ALLOC);
	    break;
	}

	if (p == head->size)			/* Reached end of this memory chunk? */
	    break;
	if (p > head->size)			/* Went past the chunk? This must never happen! */
	    Alert(AN_MemCorrupt);
    }

    ReleaseSemaphore(&head->sem);
    return ret;
}

/* Free 'size' bytes starting from address 'addr' in the MemHeader mh */
void mm_Free(struct MemHeader *mh, APTR addr, IPTR size)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    /* Calculate number of the starting page within the region */
    IPTR first = (addr - head->start) / head->pageSize;
    IPTR align = head->pageSize - 1;
    IPTR pages;

    /* Pad up size and convert it to number of pages */
    size = (size + align) & ~align;
    pages = size / head->pageSize;

    ObtainSemaphore(&head->sem);
    
    /* Set block state to free */
    SetBlockState(head, first, pages, P_FREE);

    /* Update free memory counter */
    mh->mh_Free += size;

    ReleaseSemaphore(&head->sem);
}

/*
 * Iniialize memory management in a given MemHeader.
 * This routine takes into account only mh_Lower and mh_Upper, the rest is
 * ignored.
 * TODO: Currently it's assumed that the MemHeader itself is placed in the beginning
 * of the region. In future this may be not true.
 */
void mm_Init(struct MemHeader *mh, ULONG pageSize)
{
    struct BlockHeader *head;
    IPTR align;
    APTR end;
    IPTR memsize;
    IPTR mapsize;
    IPTR p;
    UBYTE free;

    /*
     * Currently we assume the struct MemHeader to be in the beginning
     * our our region.
     */
    head  = (APTR)mh + MEMHEADER_TOTAL;
    align = pageSize - 1;

    /* Fill in the BlockHeader */
    head->mc.mc_Next  = NULL;
    head->mc.mc_Bytes = 0;
    head->pageSize    = pageSize;

    /*
     * Page-align boundaries. 
     * We intentionally make it start pointing to the previous page,
     * we'll jump to the next page later, in the loop.
     */
    head->start = (APTR)((IPTR)head->map & ~align);
    end = (APTR)(((IPTR)mh->mh_Upper + 1) & ~align);

    D(bug("[mm_Init] MemHeader 0x%p, BlockHeader 0x%p, usable 0x%p - 0x%p\n", mh, head, head->start, end));

    do
    {
    	/* Skip one page. This reserves some space (one page or less) for allocations map. */
    	head->start += pageSize;
    	/* Calculate resulting map size */
	mapsize = (head->start - (APTR)head->map) / sizeof(ULONG);
	/* Calculate number of free bytes and pages */
	memsize = end - head->start;
    	head->size = memsize / pageSize;
	/*
	 * Repeat the operation if there's not enough memory for allocations map.
	 * This will take one more page from the area and use it for the map.
	 */
    } while (mapsize < head->size);

    D(bug("[mm_Init] Got %u usable pages\n", head->size));

    /* Mark all pages as free */
    p = head->size;
    free = 1;
    do {
    	head->map[--p] = free;
	if (free < 127)
	    free++;
    } while (p > 0);

    /* Set BlockHeader pointer and free space counter */
    mh->mh_First = &head->mc;
    mh->mh_Free  = memsize;
}

/*
 * Apply memory protection to the MemHeader.
 * This is a separate routine because MMU by itself requires some memory to place its
 * control structures, and they need to be allocated using already working allocator.
 * Only once MMU is up and running, we can apply protection to our memory.
 */
void mm_Protect(struct MemHeader *mh, struct KernelBase *KernelBase)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;

    InitSemaphore(&head->sem);

    /* TODO */
}

#define SET_LARGEST(ptr, val)	\
    if (ptr)			\
    {				\
	if (val > *ptr)		\
	    *ptr = val;		\
    }

#define SET_SMALLEST(ptr, val)	\
    if (ptr)			\
    {				\
	if (*ptr)		\
	{			\
	    if (val < *ptr)	\
		*ptr = val;	\
	}			\
	else			\
	    *ptr = val;		\
    }

/* Get statistics from the specified MemHeader */
void mm_StatMemHeader(struct MemHeader *mh, const struct TagItem *query, struct KernelBase *KernelBase)
{
    struct TagItem *tag;
    IPTR *largest_alloc  = NULL;
    IPTR *smallest_alloc = NULL;
    IPTR *largest_free   = NULL;
    IPTR *smallest_free  = NULL;
    IPTR *num_alloc      = NULL;
    IPTR *num_free       = NULL;
    BOOL do_traverse = FALSE;

    while ((tag = LibNextTagItem(&query)))
    {
	switch (tag->ti_Tag)
	{
	case KMS_Free:
	    *((IPTR *)tag->ti_Data) += mh->mh_Free;
	    break;

	case KMS_Total:
	    *((IPTR *)tag->ti_Data) += mh->mh_Upper - mh->mh_Lower;
	    break;

	case KMS_LargestFree:
	    largest_free = (IPTR *)tag->ti_Data;
	    do_traverse = TRUE;
	    break;

	case KMS_SmallestFree:
	    smallest_free = (IPTR *)tag->ti_Data;
	    do_traverse = TRUE;
	    break;

	case KMS_LargestAlloc:
	    largest_alloc = (IPTR *)tag->ti_Data;
	    do_traverse = TRUE;
	    break;

	case KMS_SmallestAlloc:
	    smallest_alloc = (IPTR *)tag->ti_Data;
	    do_traverse = TRUE;
	    break;
	
	case KMS_NumAlloc:
	    num_alloc = (IPTR *)tag->ti_Data;
	    do_traverse = TRUE;
	    break;

	case KMS_NumFree:
	    num_free = (IPTR *)tag->ti_Data;
	    do_traverse = TRUE;
	    break;
	}
    }

    if (do_traverse)
    {
	struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
	IPTR p;

	ObtainSemaphoreShared(&head->sem);

	for (p = 0; p < head->size; )
	{
	    /* Get total size and state of the current block */
	    IPTR blksize = 0;
	    page_t blkstate = P_STATUS(head->map[p]);

	    do
	    {
		UBYTE cnt = P_COUNT(head->map[p]);	/* Get (partial) block length */

		blksize += cnt;				/* Add length to total count */
		p += cnt;				/* Advance past the block    */

		if (p == head->size)			/* Reached end of this memory chunk ? */
		    break;
		if (p > head->size)			/* Went past the chunk? This must never happen! */
		    Alert(AN_MemCorrupt);
	    } while (P_STATUS(head->map[p]) == blkstate);

	    blksize *= head->pageSize;			/* Convert to bytes */

	    if (blkstate == P_ALLOC)
	    {
		SET_LARGEST(largest_alloc, blksize);
		SET_SMALLEST(smallest_alloc, blksize);

		if (num_alloc)
		    *num_alloc += 1;
	    }
	    else
	    {
		SET_LARGEST(largest_free, blksize);
		SET_SMALLEST(smallest_free, blksize);

		if (num_free)
		    *num_free += 1;
	    }
	}

	ReleaseSemaphore(&head->sem);
    }
}
