#include <proto/kernel.h>

#include "exec_intern.h"
#include "memory.h"

/* This will compile only if KrnAllocPages() is present!!! */
#ifdef KrnAllocPages

/* Allocate a region managed by own header */
APTR AllocMemHeader(IPTR size, ULONG flags, KRN_MapAttr prot, struct ExecBase *SysBase)
{
    struct MemHeader *mh;

    mh = KrnAllocPages(size, flags, prot);
    if (mh)
    {
    	size -= MEMHEADER_TOTAL;

	/* Initialize new MemHeader */
	mh->mh_Node.ln_Name	= NULL;
	mh->mh_Node.ln_Type	= NT_MEMORY;
	mh->mh_Attributes	= TypeOfMem(mh);
	mh->mh_Lower 	    	= (APTR)mh + MEMHEADER_TOTAL;
	mh->mh_Upper 	    	= mh->mh_Lower + size - 1;
	mh->mh_First	    	= (struct MemChunk *)mh->mh_Lower;
	mh->mh_Free  	    	= size;
	mh->mh_First->mc_Next 	= NULL;
	mh->mh_First->mc_Bytes  = size;
    }
    return mh;
}

/* Free a region allocated by AllocMemHeader() */
void FreeMemHeader(APTR addr, struct ExecBase *SysBase)
{
    KrnFreePages(addr, ((struct MemHeader *)addr)->mh_Upper - addr + 1);
}

/* Allocate puddle of a requested size with given flags and add it to the pool */
APTR AllocPuddle(struct Pool *pool, IPTR size, ULONG flags, struct ExecBase *SysBase)
{
    APTR ret;
    KRN_MapAttr prot = MAP_Readable|MAP_Writable|MAP_Executable;

    ret = AllocMemHeader(size, flags, prot, SysBase);
    if (ret)
    	AddTail((struct List *)&pool->PuddleList, ret);

    return ret;
}

/*
 * Allocate memory with given physical properties from the given pool.
 * Our pools can be mixed. This means that different puddles from the pool can have
 * different physical flags. For example the same pool can contain both CHIP and FAST memory.
 * This is done in order to provide a single system default pool for all types of memory.
 */
APTR InternalAllocPooled(APTR poolHeader, IPTR memSize, ULONG flags, struct ExecBase *SysBase)
{
    struct ProtectedPool *pool = poolHeader + MEMHEADER_TOTAL;
    APTR    	    	 ret = NULL;
    struct MemHeader *mh;

    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ObtainSemaphore(&pool->sem);
    }

    if (memSize > pool->pool.ThreshSize)
    {
        /*
         * If the memSize is bigger than the ThreshSize allocate seperately.
         * Our allocation size is always page-aligned, so we will likely have
         * some unused space beyond the requested region. We will make use of
         * it for our pool, in order to to this we actually allocate another
         * puddle, of increased size. It will also contain the MemHeader.
         */
	IPTR align = PrivExecBase(SysBase)->PageSize - 1;
	/* Get enough memory for the memory block including the header. */
        IPTR blockSize = memSize + MEMHEADER_TOTAL;

        /* Align the size up to page boundary */
        blockSize = (blockSize + align) & ~align;

	mh = AllocPuddle(&pool->pool, blockSize, flags, SysBase);
	if (mh)
	    /* Allocate the requested memory from the new header */
	    ret = Allocate(mh, memSize);
    }
    else
    {
	/* Follow the list of MemHeaders */
	mh = (struct MemHeader *)pool->pool.PuddleList.mlh_Head;
	for(;;)
	{
	    /* Are there no more MemHeaders? */
	    if(mh->mh_Node.ln_Succ==NULL)
	    {
	    	/* Get a new one */
	    	mh = AllocPuddle(&pool->pool, pool->pool.PuddleSize, flags, SysBase);

		/* No memory left? */
		if(mh == NULL)
		    goto done;

		/* Fall through to get the memory */
	    }
	    else
	    {
	    	/* Ignore existing MemHeaders with memory type that differ from the requested ones */
	    	if (flags & MEMF_PHYSICAL_MASK & ~mh->mh_Attributes)
	    	    continue;
	    }

	    /* Try to get the memory */
	    ret = Allocate(mh, memSize);

	    /* Got it? */
	    if(ret != NULL)
            {
            	/* If this is not the first MemHeader and it has some free space, move it to the head */
            	if (mh->mh_Node.ln_Pred != NULL && mh->mh_Free > 32)
            	{
                    Remove((struct Node *)mh);
                    AddHead((struct List *)&pool->pool.PuddleList, (struct Node *)&mh->mh_Node);
            	}

                break;
            }

	    /* No. Try next MemHeader */
	    mh = (struct MemHeader *)mh->mh_Node.ln_Succ;
	}
	/* Allocate does not clear the memory! */
	if(flags & MEMF_CLEAR)
	{
	    IPTR *p= ret;

	    /* Round up (clearing IPTRs is faster than just bytes) */
	    memSize = (memSize + sizeof(IPTR) - 1) / sizeof(IPTR);

	    /* NUL the memory out */
	    while(memSize--)
		*p++=0;
	}
    }

done:
    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ReleaseSemaphore(&pool->sem);
    }
    
    /* Everything fine */
    return ret;
}

#endif
