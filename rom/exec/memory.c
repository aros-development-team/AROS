#include <aros/debug.h>
#include <proto/kernel.h>

#include "exec_intern.h"
#include "memory.h"

#define DMH(x)

/* Transition period: use AllocMem()/FreeMem() as base allocator */
#undef KrnAllocPages
#undef KrnFreePages

/* Find MemHeader to which address belongs */
struct MemHeader *FindMem(APTR address, struct ExecBase *SysBase)
{
    struct MemHeader *mh;

    /* Nobody should change the memory list now. */
    Forbid();

    /* Follow the list of MemHeaders */
    mh = (struct MemHeader *)SysBase->MemList.lh_Head;

    while(mh->mh_Node.ln_Succ != NULL)
    {
	/* Check if this MemHeader fits */
	if(address >= mh->mh_Lower && address <= mh->mh_Upper)
	{
	    /* Yes. Return it. */
	    Permit();
	    return mh;
	}

	/* Go to next MemHeader */
	mh = (struct MemHeader *)mh->mh_Node.ln_Succ;
    }

    Permit();
    return NULL;
}

/*
 * Allocate block from the given MemHeader.
 * From the requirements it takes only MEMF_REVERSE flag in order to
 * know allocation direction.
 * This routine can be called with SysBase = NULL.
 */
APTR stdAlloc(struct MemHeader *mh, ULONG byteSize, ULONG requirements, struct ExecBase *SysBase)
{
    struct MemChunk *mc=NULL, *p1, *p2;
    
    /*
        The free memory list is only single linked, i.e. to remove
        elements from the list I need node's predessor. For the
        first element I can use mh->mh_First instead of a real predessor.
    */
    p1 = (struct MemChunk *)&mh->mh_First;
    p2 = p1->mc_Next;

    /* Is there anything in the list? */
    if (p2 != NULL)
    {
        /* Then follow it */
        for (;;)
        {
#if !defined(NO_CONSISTENCY_CHECKS)
            /* Consistency check: Check alignment restrictions */
            if (((IPTR)p2|(IPTR)p2->mc_Bytes) & (MEMCHUNK_TOTAL-1))
	    {
		if (SysBase)
		{
		    bug("[MM] Chunk allocator error\n");
		    bug("[MM] Attempt to allocate %u bytes from MemHeader 0x%p\n", byteSize, mh);
		    bug("[MM] Misaligned chunk at 0x%p (%u bytes)\n", p2, p2->mc_Bytes);

		    Alert(AN_MemCorrupt|AT_DeadEnd);
		}
		return NULL;
	    }
#endif
            
            /* Check if the current block is large enough */
            if(p2->mc_Bytes>=byteSize)
            {
                /* It is. */
                mc=p1;
                /* Use this one if MEMF_REVERSE is not set.*/
                if(!(requirements&MEMF_REVERSE))
                    break;
                /* Else continue - there may be more to come. */
            }

            /* Go to next block */
            p1=p2;
            p2=p1->mc_Next;

            /* Check if this was the end */
            if(p2==NULL)
                break;
#if !defined(NO_CONSISTENCY_CHECKS)
            /*
                Consistency check:
                If the end of the last block+1 is bigger or equal to
                the start of the current block something must be wrong.
            */
            if((UBYTE *)p2<=(UBYTE *)p1+p1->mc_Bytes)
	    {
		if (SysBase)
		{
		    bug("[MM] Chunk allocator error\n");
		    bug("[MM] Attempt to allocate %u bytes from MemHeader 0x%p\n", byteSize, mh);
		    bug("[MM] Overlapping chunks 0x%p (%u bytes) and 0x%p (%u bytes)\n", p1, p1->mc_Bytes, p2, p2->mc_Bytes);

		    Alert(AN_MemCorrupt|AT_DeadEnd);
		}
		return NULL;
	    }
#endif
        }
        
        /* Something found? */
        if (mc != NULL)
        {
            /*
                Remember: if MEMF_REVERSE is set
                p1 and p2 are now invalid.
            */
            p1=mc;
            p2=p1->mc_Next;

            /* Remove the block from the list and return it. */
            if(p2->mc_Bytes == byteSize)
            {
                /* Fits exactly. Just relink the list. */
                p1->mc_Next = p2->mc_Next;
                mc          = p2;
            }
            else
            {
                if(requirements & MEMF_REVERSE)
                {
                    /* Return the last bytes. */
                    p1->mc_Next=p2;
                    mc=(struct MemChunk *)((UBYTE *)p2+p2->mc_Bytes-byteSize);
                }
                else
                {
                    /* Return the first bytes. */
                    p1->mc_Next=(struct MemChunk *)((UBYTE *)p2+byteSize);
                    mc=p2;
                }
                
                p1           = p1->mc_Next;
                p1->mc_Next  = p2->mc_Next;
                p1->mc_Bytes = p2->mc_Bytes-byteSize;
            }
            
            mh->mh_Free -= byteSize;
        }
    }
    
    return mc;
}

/* Backwards compatibility for old ports */
#ifndef KrnAllocPages
#define KrnAllocPages(addr, size, flags) AllocMem(size, flags & ~MEMF_SEM_PROTECTED)
#define KrnFreePages(addr, size)         FreeMem(addr, size)
#endif

/* Allocate a region managed by own header */
APTR AllocMemHeader(IPTR size, ULONG flags, struct ExecBase *SysBase)
{
    struct MemHeader *mh;

    mh = KrnAllocPages(NULL, size, flags);
    DMH(bug("[AllocMemHeader] Allocated %u bytes at 0x%p\n", size, mh));

    if (mh)
    {
        struct MemHeader *orig = FindMem(mh, SysBase);

    	size -= MEMHEADER_TOTAL;

	/*
	 * Initialize new MemHeader.
	 * Inherit attributes from system MemHeader from which
	 * our chunk was allocated.
	 */
	mh->mh_Node.ln_Type	= NT_MEMORY;
	mh->mh_Node.ln_Pri      = orig->mh_Node.ln_Pri;
	mh->mh_Attributes	= orig->mh_Attributes;
	mh->mh_Lower 	    	= (APTR)mh + MEMHEADER_TOTAL;
	mh->mh_Upper 	    	= mh->mh_Lower + size - 1;
	mh->mh_First	    	= mh->mh_Lower;
	mh->mh_Free  	    	= size;

	/* Create the first (and the only) MemChunk */
	mh->mh_First->mc_Next 	= NULL;
	mh->mh_First->mc_Bytes  = size;
    }
    return mh;
}

/* Free a region allocated by AllocMemHeader() */
void FreeMemHeader(APTR addr, struct ExecBase *SysBase)
{
    ULONG size = ((struct MemHeader *)addr)->mh_Upper - addr + 1;

    DMH(bug("[FreeMemHeader] Freeing %u bytes at 0x%p\n", size, addr));
    KrnFreePages(addr, size);
}

/*
 * Allocate memory with given physical properties from the given pool.
 * Our pools can be mixed. This means that different puddles from the
 * pool can have different physical flags. For example the same pool
 * can contain puddles from both CHIP and FAST memory. This is done in
 * order to provide a single system default pool for all types of memory.
 */
APTR InternalAllocPooled(APTR poolHeader, IPTR memSize, ULONG flags, struct ExecBase *SysBase)
{
    struct ProtectedPool *pool = poolHeader + MEMHEADER_TOTAL;
    APTR    	    	 ret = NULL;
    struct MemHeader *mh;

    D(bug("[exec] InternalAllocPooled(0x%p, %u, 0x%08X), header 0x%p\n", poolHeader, memSize, flags, pool));

    /*
     * Memory blocks allocated from the pool store pointers to the MemHeader they were
     * allocated from. This is done in order to avoid slow lookups in InternalFreePooled().
     * This is done in AllocVec()-alike manner, the pointer is placed right before the block.
     */
    memSize += sizeof(struct MemHeader *);

    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ObtainSemaphore(&pool->sem);
    }

    /* Follow the list of MemHeaders */
    mh = (struct MemHeader *)pool->pool.PuddleList.mlh_Head;
    for(;;)
    {
	/* Are there no more MemHeaders? */
	if (mh->mh_Node.ln_Succ==NULL)
	{
	    /*
	     * Get a new one.
	     * Usually we allocate puddles of default size, specified during
	     * pool creation. However we can be asked to allocate block whose
	     * size will be larger than default puddle size.
	     * Previously this was handled by threshSize parameter. In our new
	     * implementation we just allocate enlarged puddle. This is done
	     * in order not to waste page tails beyond the allocated large block.
	     * These tails will be used for our pool too. Their size is smaller
	     * than page size but they still perfectly fit for small allocations
	     * (the primary use for pools).
	     * Since our large block is also a puddle, it will be reused for our
	     * pool when the block is freed. It can also be reused for another
	     * large allocation, if it fits in.
	     * Our final puddle size still includes MEMHEADER_TOTAL in any case.
	     */
	    IPTR puddleSize = pool->pool.PuddleSize;

	    if (memSize > puddleSize - MEMHEADER_TOTAL)
	    {
		IPTR align = PrivExecBase(SysBase)->PageSize - 1;

		puddleSize = memSize + MEMHEADER_TOTAL;
		/* Align the size up to page boundary */
		puddleSize = (puddleSize + align) & ~align;
	    }

	    mh = AllocMemHeader(puddleSize, flags, SysBase);
	    D(bug("[InternalAllocPooled] Allocated new puddle 0x%p, size %u\n", mh, puddleSize));

	    /* No memory left? */
	    if(mh == NULL)
		break;

	    /* Add the new puddle to our pool */
	    mh->mh_Node.ln_Name = (STRPTR)pool;
	    Enqueue((struct List *)&pool->pool.PuddleList, &mh->mh_Node);

	    /* Fall through to get the memory */
	}
	else
	{
	    /* Ignore existing MemHeaders with memory type that differ from the requested ones */
	    if (flags & MEMF_PHYSICAL_MASK & ~mh->mh_Attributes)
	    {
		D(bug("[InternalAllocPooled] Wrong flags for puddle 0x%p (wanted 0x%08X, have 0x%08X\n", flags, mh->mh_Attributes));
	    	continue;
	    }
	}

	/* Try to get the memory */
	ret = Allocate(mh, memSize);
	D(bug("[InternalAllocPooled] Allocated memory at 0x%p from puddle 0x%p\n", ret, mh));

	/* Got it? */
	if (ret != NULL)
        {
            /*
	     * If this is not the first MemHeader and it has some free space,
	     * move it forward (so that the next allocation will attempt to use it first).
	     * We use Enqueue() because we still sort MemHeaders according to their priority
	     * (which they inherit from system MemHeaders).
	     *
	     * TODO: implement own Enqueue() routine with secondary sorting by mh_Free.
	     * This will allow to implement best-match algorithm (so that puddles with
	     * smaller free space will be picked up first). This way the smallest allocations
	     * will reuse smallest chunks instead of fragmenting large ones.
	     */
            if (mh->mh_Node.ln_Pred != NULL && mh->mh_Free > 32)
            {
		D(bug("[InternalAllocPooled] Re-sorting puddle list\n"));
                Remove(&mh->mh_Node);
                Enqueue((struct List *)&pool->pool.PuddleList, &mh->mh_Node);
            }

            break;
        }

	/* No. Try next MemHeader */
	mh = (struct MemHeader *)mh->mh_Node.ln_Succ;
    }

    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ReleaseSemaphore(&pool->sem);
    }

    if (ret)
    {
	/* Remember where we were allocated from */
	*((struct MemHeader **)ret) = mh;
	ret += sizeof(struct MemHeader *);

	/* Allocate does not clear the memory! */
	if (flags & MEMF_CLEAR)
	    memset(ret, 0, memSize - sizeof(struct MemHeader *));
    }

    /* Everything fine */
    return ret;
}

/*
 * This is a pair to InternalAllocPooled()
 * This code separated from FreePooled() in order to provide compatibility with various
 * memory tracking patches. If some exec code calls InternalAllocPooled() directly
 * (AllocMem() will do it), it has to call also InternalFreePooled() directly.
 * Our chunks remember from which pool they came, so we don't need a pointer to pool
 * header here. This will save us from headaches in future FreeMem() implementation.
 */
void InternalFreePooled(APTR memory, IPTR memSize, struct ExecBase *SysBase)
{
    struct MemHeader *mh;
    APTR freeStart;
    IPTR freeSize;

    D(bug("[exec] InternalFreePooled(0x%p, 0x%p, %u)\n", poolHeader, memory, memSize));

    if (!memory || !memSize) return;

    /* Get MemHeader pointer. It is stored right before our block. */
    freeStart = memory - sizeof(struct MemHeader *);
    freeSize = memSize + sizeof(struct MemHeader *);
    mh = *((struct MemHeader **)freeStart);

    /* Verify that MemHeader pointer is correct */
    if ((mh->mh_Node.ln_Type != NT_MEMORY) ||
	(freeStart < mh->mh_Lower) || (freeStart + memSize > mh->mh_Upper + 1))
    {
    	/*
	 * Something is wrong.
	 * TODO: the following should actually be printed as part of the alert.
	 * In future there should be some kind of "alert context". CPU alerts
	 * (like illegal access) should remember CPU context there. Memory manager
	 * alerts (like this one) should remember some own information.
	 */
        bug("[MM] Pool manager error\n");
	bug("[MM] Attempt to free %u bytes at 0x%p\n", memSize, memory);
	bug("[MM] The chunk does not belong to a pool\n");

	Alert(AT_Recovery | AN_MemCorrupt);
    }
    else
    {
	struct ProtectedPool *pool = (struct ProtectedPool *)mh->mh_Node.ln_Name;
	IPTR size;

	if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
	{
	    ObtainSemaphore(&pool->sem);
	}

	size = mh->mh_Upper - mh->mh_Lower + 1;
	D(bug("[FreePooled] Allocated from puddle 0x%p, size %u\n", mh, size));

	/* Free the memory. */
	Deallocate(mh, freeStart, freeSize);
	D(bug("[FreePooled] Deallocated chunk, %u free bytes in the puddle\n", mh->mh_Free));

	/* Is this MemHeader completely free now? */
	if (mh->mh_Free == size)
	{
	    D(bug("[FreePooled] Puddle is empty, giving back to the system\n"));

	    /* Yes. Remove it from the list. */
	    Remove(&mh->mh_Node);
	    /* And free it. */
	    FreeMemHeader(mh, SysBase);
	}
	/* All done. */

	if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
	{
    	    ReleaseSemaphore(&pool->sem);
	}
    }
}
