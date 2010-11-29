#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include "memory_intern.h"

#include "../exec/memory.h"	/* needed for MEMHEADER_TOTAL */

#define D(x)

/*
 * Create MemHeader structure for the specified RAM region.
 * The header will be placed in the beginning of the region itself.
 * The header will NOT be added to the memory list!
 */
void krnCreateMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags)
{
    /* The MemHeader itself does not have to be aligned */
    struct MemHeader *mh = start;

    mh->mh_Node.ln_Succ    = NULL;
    mh->mh_Node.ln_Pred    = NULL;
    mh->mh_Node.ln_Type    = NT_MEMORY;
    mh->mh_Node.ln_Name    = (STRPTR)name;
    mh->mh_Node.ln_Pri     = pri;
    mh->mh_Attributes      = flags;
    /* The first MemChunk needs to be aligned. We do it by adding MEMHEADER_TOTAL. */
    mh->mh_First           = start + MEMHEADER_TOTAL;
    mh->mh_First->mc_Next  = NULL;
    mh->mh_First->mc_Bytes = size - MEMHEADER_TOTAL;

    /*
     * mh_Lower and mh_Upper are informational only. Since our MemHeader resides
     * inside the region it describes, the region includes MemHeader.
     */
    mh->mh_Lower           = start;
    mh->mh_Upper           = start + size - 1;
    mh->mh_Free            = mh->mh_First->mc_Bytes;
}

/*
 * Create informational MemHeader for ROM region.
 * The header will be allocated inside another region, specified by 'ram' parameter.
 * It will be not possible to allocate memory from the created MemHeader.
 * The header will be added to the memory list.
 * This routine uses exec.library/Allocate() for memory allocation, so it is safe
 * to use before exec.library and kernel.resource memory management is initialized.
 */
struct MemHeader *krnCreateROMHeader(struct MemHeader *ram, CONST_STRPTR name, APTR start, APTR end)
{
    struct MemHeader *mh = Allocate(ram, sizeof(struct MemHeader));

    if (mh)
    {
	mh->mh_Node.ln_Type = NT_MEMORY;
	mh->mh_Node.ln_Name = (STRPTR)name;
	mh->mh_Node.ln_Pri = -128;
	mh->mh_Attributes = MEMF_KICK;
	mh->mh_First = NULL;
	mh->mh_Lower = start;
	mh->mh_Upper = end;
	mh->mh_Free = 0;                        /* Never allocate from this chunk! */
	Enqueue(&SysBase->MemList, &mh->mh_Node);
    }

    return mh;
}

/* Allocate 'size' bytes from MemHeader mh. Returns number of the first page */
APTR krnAllocate(struct MemHeader *mh, IPTR size, struct KernelBase *KernelBase)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    APTR addr = NULL;
    IPTR align = KernelBase->kb_PageSize - 1;
    IPTR pages;
    IPTR p;
    IPTR candidate, candidate_size;

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
        UBYTE cnt = 1;

	D(bug("[krnAllocate] Allocating %u pages starting from %u\n", pages, candidate));
	p = candidate + pages;
	do
	{
	    head->map[--p] = cnt | P_ALLOC;
	    INC_COUNT(cnt);
	} while (p > 0);

	/* Calculate starting address of the first page */
	addr = head->start + candidate * KernelBase->kb_PageSize;
	/* Update free memory counter */
	mh->mh_Free -= size;
    }

    ReleaseSemaphore(&head->sem);

    return addr;
}

/* Free 'size' bytes starting from address 'addr' in the MemHeader mh */
void krnFree(struct MemHeader *mh, APTR addr, IPTR size, struct KernelBase *KernelBase)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    /* Calculate number of the starting page within the region */
    IPTR first = (addr - head->start) / KernelBase->kb_PageSize;
    IPTR align = KernelBase->kb_PageSize - 1;
    UBYTE free = 0;
    IPTR pages;
    IPTR p;

    /* Pad up size and convert it to number of pages */
    size = (size + align) & ~align;
    pages = size / KernelBase->kb_PageSize;

    ObtainSemaphore(&head->sem);
    
    /* Get number of already free pages next to our region */
    p = first + pages;
    if (p > head->size)
    	/* If we claim we want to free more pages than our MemHeader has, this is bad */
    	Alert(AN_BadFreeAddr);
    else if (p < head->size)
    {
        /*
         * If we have some free pages next to our region, pick up
    	 * free pages count from the map entry of the next page.
    	 */
	if (P_STATUS(head->map[p]) == P_FREE)
	    free = P_COUNT(head->map[p]);
    }

    /*
     * Mark pages as free. We free pages from last to the first, and every
     * freed pages adds 1 to the count of free pages.
     */
    do
    {
    	head->map[--p] = free;
	INC_COUNT(free);
    } while (p > first);

    /*
     * If there are free pages preceding just freed region, we need to update their
     * free space counter, so as free blocks get merged.
     * Updating is not strictly necessary, but it helps to optimize searching process.
     */
    do {
    	if (P_STATUS(head->map[--p]) == P_ALLOC)
    	    break;

	head->map[p] = free;
	INC_COUNT(free);
    } while (p > 0);

    /* Update free memory counter */
    mh->mh_Free += size;

    ReleaseSemaphore(&head->sem);
}
