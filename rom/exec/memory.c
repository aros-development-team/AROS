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
	mh->mh_Attributes	= flags;
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

/* Allocate puddle of a requested size and add it to the pool */
APTR AllocPuddle(struct Pool *pool, IPTR size, struct ExecBase *SysBase)
{
    APTR ret;
    KRN_MapAttr prot = MAP_Readable|MAP_Writable|MAP_Executable;

    ret = AllocMemHeader(size, pool->Requirements, prot, SysBase);
    if (ret)
    	AddTail((struct List *)&pool->PuddleList, ret);

    return ret;
}

#endif
