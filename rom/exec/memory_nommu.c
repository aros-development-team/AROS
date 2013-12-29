/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: System memory allocator for MMU-less systems.
          Used also as boot-time memory allocator on systems with MMU.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"

APTR nommu_AllocMem(IPTR byteSize, ULONG flags, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    APTR res = NULL;
    struct MemHeader *mh;
    ULONG requirements = flags & MEMF_PHYSICAL_MASK;

    /* Protect memory list against other tasks */
    MEM_LOCK;

    /* Loop over MemHeader structures */
    ForeachNode(&SysBase->MemList, mh)
    {
        /*
         * Check for the right requirements and enough free memory.
         * The requirements are OK if there's no bit in the
         * 'attributes' that isn't set in the 'mh->mh_Attributes'.
         */
        if ((requirements & ~mh->mh_Attributes)
                || mh->mh_Free < byteSize)
            continue;

        if (IsManagedMem(mh))
        {
            struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

            if (mhe->mhe_Alloc)
                res = mhe->mhe_Alloc(mhe, byteSize, &flags);
        }
        else
        {
            res = stdAlloc(mh, mhac_GetSysCtx(mh, SysBase), byteSize, flags, loc, SysBase);
        }
        if (res)
            break;
    }

    MEM_UNLOCK;

    return res;
}

APTR nommu_AllocAbs(APTR location, IPTR byteSize, struct ExecBase *SysBase)
{
    struct MemHeader *mh;
    APTR ret = NULL;
    APTR endlocation = location + byteSize;

    /* Protect the memory list from access by other tasks. */
    MEM_LOCK;

    /* Loop over MemHeader structures */
    ForeachNode(&SysBase->MemList, mh)
    {
        if (IsManagedMem(mh))
        {
            struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;
            if (mhe->mhe_InBounds(mhe, location, endlocation))
            {
                if (mhe->mhe_AllocAbs)
                {
                    void * ret = mhe->mhe_AllocAbs(mhe, byteSize, location);

                    MEM_UNLOCK;

                    return ret;
                }
            }
        }
        else
            if (mh->mh_Lower <= location && mh->mh_Upper >= endlocation)
                break;
    }
    
    /* If no header was found which matched the requirements, just give up. */
    if (mh->mh_Node.ln_Succ)
    {
        struct MemChunk *p1, *p2, *p3, *p4;
        
        /* Align size to the requirements */
        byteSize += (IPTR)location&(MEMCHUNK_TOTAL - 1);
        byteSize  = (byteSize + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1);
        
        /* Align the location as well */
        location=(APTR)((IPTR)location & ~(MEMCHUNK_TOTAL-1));
        
        /* Start and end(+1) of the block */
        p3=(struct MemChunk *)location;
        p4=(struct MemChunk *)((UBYTE *)p3+byteSize);
        
        /*
            The free memory list is only single linked, i.e. to remove
            elements from the list I need the node's predessor. For the
            first element I can use freeList->mh_First instead of a real
            predecessor.
        */
        p1 = (struct MemChunk *)&mh->mh_First;
        p2 = p1->mc_Next;

        /* Follow the list to find a chunk with our memory. */
        while (p2 != NULL)
        {
#if !defined(NO_CONSISTENCY_CHECKS)
            /*
             * Memory list consistency checks.
             * 1. Check alignment restrictions
             */
            if (((IPTR)p2|(IPTR)p2->mc_Bytes) & (MEMCHUNK_TOTAL-1))
            {
                if (SysBase && SysBase->DebugAROSBase)
                {
                    bug("[MM] Chunk allocator error\n");
                    bug("[MM] Attempt to allocate %lu bytes at 0x%p from MemHeader 0x%p\n", byteSize, location, mh);
                    bug("[MM] Misaligned chunk at 0x%p (%u bytes)\n", p2, p2->mc_Bytes);

                    Alert(AN_MemoryInsane|AT_DeadEnd);
                }
                break;
            }

            /* 2. Check against overlapping blocks */
            if (p2->mc_Next && ((UBYTE *)p2 + p2->mc_Bytes >= (UBYTE *)p2->mc_Next))
            {
                if (SysBase && SysBase->DebugAROSBase)
                {
                    bug("[MM] Chunk allocator error\n");
                    bug("[MM] Attempt to allocate %lu bytes at 0x%p from MemHeader 0x%p\n", byteSize, location, mh);
                    bug("[MM] Overlapping chunks 0x%p (%u bytes) and 0x%p (%u bytes)\n", p2, p2->mc_Bytes, p2->mc_Next, p2->mc_Next->mc_Bytes);

                    Alert(AN_MemoryInsane|AT_DeadEnd);
                }
                break;
            }
#endif

            /* Found a chunk that fits? */
            if((UBYTE *)p2+p2->mc_Bytes>=(UBYTE *)p4&&p2<=p3)
            {
                /* Since AllocAbs allocations never allocate/update a ctx, they need to clear it if it exists */
                mhac_ClearSysCtx(mh, SysBase);

                /* Check if there's memory left at the end. */
                if((UBYTE *)p2+p2->mc_Bytes!=(UBYTE *)p4)
                {
                    /* Yes. Add it to the list */
                    p4->mc_Next  = p2->mc_Next;
                    p4->mc_Bytes = (UBYTE *)p2+p2->mc_Bytes-(UBYTE *)p4;
                    p2->mc_Next  = p4;
                }

                /* Check if there's memory left at the start. */
                if(p2!=p3)
                    /* Yes. Adjust the size */
                    p2->mc_Bytes=(UBYTE *)p3-(UBYTE *)p2;
                else
                    /* No. Skip the old chunk */
                    p1->mc_Next=p2->mc_Next;
    
                /* Adjust free memory count */
                mh->mh_Free-=byteSize;

                /* Return the memory */
                ret = p3;
                break;
            }
            /* goto next chunk */
        
            p1=p2;
            p2=p2->mc_Next;
        }
    }

    MEM_UNLOCK;

    return ret;
}

void nommu_FreeMem(APTR memoryBlock, IPTR byteSize, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    struct MemHeader *mh;
    APTR blockEnd;

    /* It is legal to free zero bytes */
    if (!byteSize)
        return;

    blockEnd = memoryBlock + byteSize;

    /* Protect the memory list from access by other tasks. */
    MEM_LOCK;

    ForeachNode(&SysBase->MemList, mh)
    {
        if (IsManagedMem(mh))
        {
            struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

            if (mhe->mhe_InBounds(mhe, memoryBlock, blockEnd))
            {
                if (mhe->mhe_Free)
                {
                    mhe->mhe_Free(mhe, memoryBlock, byteSize);

                    MEM_UNLOCK;
                    return;
                }
            }
        }
        else
        {
            /* Test if the memory belongs to this MemHeader. */
            if (mh->mh_Lower > memoryBlock || mh->mh_Upper < blockEnd)
                continue;

            stdDealloc(mh, mhac_GetSysCtx(mh, SysBase), memoryBlock, byteSize, loc, SysBase);
        }
        MEM_UNLOCK;
        ReturnVoid ("nommu_FreeMem");
    }

    MEM_UNLOCK;

#if !defined(NO_CONSISTENCY_CHECKS)
    /* Some memory that didn't fit into any MemHeader? */
    bug("[MM] Chunk allocator error\n");
    bug("[MM] Attempt to free %u bytes at 0x%p\n", byteSize, memoryBlock);
    bug("[MM] The block does not belong to any MemHeader\n");

    Alert(AN_BadFreeAddr);
#endif

    ReturnVoid ("nommu_FreeMem");
}

IPTR nommu_AvailMem(ULONG attributes, struct ExecBase *SysBase)
{
    IPTR ret = 0;
    struct MemHeader *mh;
    ULONG physFlags = attributes & MEMF_PHYSICAL_MASK;

    D(bug("[MM] nommu_AvailMem(0x%08X)\n", attributes));
    D(bug("[MM] physical memory flags: 0x%08X\n", physFlags));

    /* Nobody else should access the memory lists now. */
    MEM_LOCK_SHARED;

    ForeachNode(&SysBase->MemList, mh)
    {
        D(bug("[MM] Checking MemHeader 0x%p\n", mh));

        /*
         * The current memheader is OK if there's no bit in the
         * 'physFlags' that isn't set in the 'mh->mh_Attributes'.
         */
        if (physFlags & ~mh->mh_Attributes)
        {
            D(bug("[MM] Skipping (mh_Attributes = 0x%08X\n", mh->mh_Attributes));
            continue;
        }

        if (IsManagedMem(mh))
        {
            struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

            if (mhe->mhe_Avail)
            {
                IPTR val = mhe->mhe_Avail(mhe, attributes);

                if (attributes & MEMF_LARGEST)
                {
                    if (val > ret)
                        ret = val;
                }
                else
                    ret += val;

                continue;
            }
        }

        /* Find largest chunk? */
        if (attributes & MEMF_LARGEST)
        {
            /*
             * Yes. Follow the list of MemChunks and set 'ret' to
             * each value that is bigger than all previous ones.
             */
            struct MemChunk *mc;

            for (mc = mh->mh_First; mc; mc = mc->mc_Next)
            {
#if !defined(NO_CONSISTENCY_CHECKS)
                /*
                 * Do some constistency checks:
                 * 1. All MemChunks must be aligned to MEMCHUNK_TOTAL.
                 */
                if (((IPTR)mc | mc->mc_Bytes) & (MEMCHUNK_TOTAL-1))
                {
                    bug("[MM] Chunk allocator error in MemHeader 0x%p\n", mh);
                    bug("[MM] Misaligned chunk at 0x%p (%u bytes)\n", mc, mc->mc_Bytes);

                    Alert(AN_MemoryInsane|AT_DeadEnd);
                }
                        /*  2. The end (+1) of the current MemChunk must be lower than the start of the next one. */
                if (mc->mc_Next && ((UBYTE *)mc + mc->mc_Bytes >= (UBYTE *)mc->mc_Next))
                {
                    bug("[MM] Chunk allocator error in MemHeader 0x%p\n");
                    bug("[MM] Overlapping chunks 0x%p (%u bytes) and 0x%p (%u bytes)\n", mc, mc->mc_Bytes, mc->mc_Next, mc->mc_Next->mc_Bytes);

                    Alert(AN_MemoryInsane|AT_DeadEnd);
                }
#endif
                if (mc->mc_Bytes>ret)
                    ret=mc->mc_Bytes;
            }
        }
        else if (attributes & MEMF_TOTAL)
            /* Determine total size. */
            ret += (IPTR)mh->mh_Upper - (IPTR)mh->mh_Lower;
        else
            /* Sum up free memory. */
            ret += mh->mh_Free;
    }

    /* All done */
    MEM_UNLOCK;

    return ret;
}
