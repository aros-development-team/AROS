#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include "memory_intern.h"

#include "../exec/memory.h"	/* needed for MEMHEADER_TOTAL */

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
    ULONG align = KernelBase->kb_PageSize - 1;
    ULONG pages;
    ULONG p;

    /* Pad up size and convert it to number of pages */
    size = (size + align) & ~align;
    pages = size / KernelBase->kb_PageSize;

    ObtainSemaphore(&head->sem);

    for (p = 0; p < head->size; p++)
    {
    	/* Found block ? */
    	if (head->map[p] >= pages)
    	{
    	    ULONG i;

    	    /* Mark pages starting from p as allocated */
    	    for (i = 0; i < pages; i++)
    	    	head->map[p + i] = 0;

	    /* Calculate starting address of the first page */
	    addr = head->start + p * KernelBase->kb_PageSize;
	    /* Update free memory counter */
	    mh->mh_Free -= size;
	    break;
	}
    }

    ReleaseSemaphore(&head->sem);

    return addr;
}

/* Free 'size' bytes starting from address 'addr' in the MemHeader mh */
void krnFree(struct MemHeader *mh, APTR addr, IPTR size, struct KernelBase *KernelBase)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    /* Calculate number of the starting page within the region */
    ULONG first = (addr - head->start) / KernelBase->kb_PageSize;
    ULONG align = KernelBase->kb_PageSize - 1;
    ULONG free = 0;
    ULONG pages;
    ULONG p;

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
        /*
         * If we have some free pages next to our region, pick up
    	 * free pages count from the map entry of the next page.
    	 */
        free = head->map[p];

    /*
     * Mark pages as free. We free pages from last to the first, and every
     * freed pages adds 1 to the count of free pages.
     */
    do
    	head->map[--p] = ++free;
    while (p > first);

    /*
     * If there are free pages preceding just freed region, we need to update their
     * free space counter, so as free blocks get merged.
     */
    do {
    	if (!head->map[--p])
    	    break;
    	head->map[p] = ++free;
    } while (p > 0);

    /* Update free memory counter */
    mh->mh_Free += size;

    ReleaseSemaphore(&head->sem);
}
