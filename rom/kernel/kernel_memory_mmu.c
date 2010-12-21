/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Page-based memory allocator.
    Lang: english
*/

#include <aros/config.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_tagitems.h>
#include "memory_intern.h"

#define D(x)

/* The whole this code makes sense only with MMU support */
#if USE_MMU

/*
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
APTR krnAllocate(struct MemHeader *mh, IPTR size, ULONG flags, struct KernelBase *KernelBase)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    APTR addr = NULL;
    IPTR align = KernelBase->kb_PageSize - 1;
    IPTR pages;
    IPTR p;
    IPTR candidate, candidate_size;

    D(bug("[krnAllocate] Request for %u bytes from BlockHeader %p\n", size, head));

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
    pages = size / KernelBase->kb_PageSize;

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

	D(bug("[krnAllocate] Have %u free pages starting from %u\n", free, start));

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
		    D(bug("[krnAllocate] Old candidate %u (size %d)\n", candidate, candidate_size));
		    candidate = start;
		    candidate_size = free;
		    D(bug("[krnAllocate] New candidate %u (size %d)\n", candidate, candidate_size));
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
		    D(bug("[krnAllocate] Old candidate %u (size %d)\n", candidate, candidate_size));
		    candidate = start;
		    candidate_size = free;
		    D(bug("[krnAllocate] New candidate %u (size %d)\n", candidate, candidate_size));
		}

		/* If found exact match, we can't do better, so stop searching */
		if (free == pages)
		{
		    D(bug("[krnAllocate] Exact match\n"));
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
	    D(bug("[krnAllocate] Reached end of chunk\n"));
	    break;
	}

	D(bug("[krnAllocate] Allocated block starts at %u\n", p));
	/* Skip past the end of the allocated block */
	while (P_STATUS(head->map[p]) == P_ALLOC)
	{
	    p += P_COUNT(head->map[p]);

	    if (p == head->size)
	    {
	    	D(bug("[krnAllocate] Reached end of chunk\n"));
		break;
	    }
	    if (p > head->size)
		Alert(AN_MemCorrupt);
	}
	D(bug("[krnAllocate] Skipped up to page %u\n", p));

    } while (p < head->size);

    /* Found block ? */
    if (candidate_size != -1)
    {
	/* Mark the block as allocated */
	D(bug("[krnAllocate] Allocating %u pages starting from %u\n", pages, candidate));
	SetBlockState(head, candidate, pages, P_ALLOC);

	/* Calculate starting address of the first page */
	addr = head->start + candidate * KernelBase->kb_PageSize;
	/* Update free memory counter */
	mh->mh_Free -= size;
    }

    ReleaseSemaphore(&head->sem);

    return addr;
}

/* Allocate 'size' bytes starting at 'addr' from MemHeader mh */
APTR krnAllocAbs(struct MemHeader *mh, void *addr, IPTR size, struct KernelBase *KernelBase)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    IPTR align = KernelBase->kb_PageSize - 1;
    IPTR pages;
    IPTR start, p;
    void *ret = NULL;

    D(bug("[krnAllocate] Request for %u bytes from BlockHeader %p\n", size, head));

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
    pages = size / KernelBase->kb_PageSize;

    ObtainSemaphore(&head->sem);

    /* Get start page number */
    start = (addr - head->start) / KernelBase->kb_PageSize;

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
void krnFree(struct MemHeader *mh, APTR addr, IPTR size, struct KernelBase *KernelBase)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    /* Calculate number of the starting page within the region */
    IPTR first = (addr - head->start) / KernelBase->kb_PageSize;
    IPTR align = KernelBase->kb_PageSize - 1;
    IPTR pages;

    /* Pad up size and convert it to number of pages */
    size = (size + align) & ~align;
    pages = size / KernelBase->kb_PageSize;

    ObtainSemaphore(&head->sem);
    
    /* Set block state to free */
    SetBlockState(head, first, pages, P_FREE);

    /* Update free memory counter */
    mh->mh_Free += size;

    ReleaseSemaphore(&head->sem);
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
void krnStatMemHeader(struct MemHeader *mh, const struct TagItem *query)
{
    struct TagItem *tag;
    IPTR *largest_alloc  = NULL;
    IPTR *smallest_alloc = NULL;
    IPTR *largest_free   = NULL;
    IPTR *smallest_free  = NULL;
    IPTR *num_alloc      = NULL;
    IPTR *num_free       = NULL;
    BOOL do_traverse = FALSE;

    while ((tag = krnNextTagItem(&query)))
    {
	switch (tag->ti_Tag)
	{
	case KMS_Free:
	    *((IPTR *)tag->ti_Data) += mh->mh_Free;
	    break;

	case KMS_Total:
	    *((IPTR *)tag->ti_Data) += mh->mh_Upper - mh->mh_Lower + 1;
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

	ObtainSemaphore(&head->sem);

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

#endif
