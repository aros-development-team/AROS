/*
    Copyright (C) 1995-2018, The AROS Development Team. All rights reserved.

    Desc: System memory allocator for MMU-less systems.
          Used also as boot-time memory allocator on systems with MMU.
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

    /*
     * Hold MemListSpinLock in shared mode for the list walk. Per-MemHeader
     * mutations are serialized by mh_SpinLock inside stdAlloc.
     * AddMemList still takes the exclusive lock.
     */
    MEM_LOCK_SHARED;

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
            {
                /*
                 * The managed allocator (e.g. TLSF) only serialises itself
                 * when the header is MEMF_SEM_PROTECTED. The system heap is
                 * not, so take the per-MemHeader spinlock here - exactly as
                 * stdAlloc does for the plain path below - otherwise
                 * concurrent AllocMem/FreeMem from other cores corrupt the
                 * free list. MemListSpinLock is held shared, so the list walk
                 * stays parallel; this only serialises this one header.
                 * SEM_PROTECTED headers obtain a semaphore internally and
                 * may Wait(), so they must NOT be entered with a spinlock
                 * held (also as stdAlloc does).
                 */
#if defined(__AROSEXEC_SMP__)
                if (!((IPTR)mh->mh_First & MEMF_SEM_PROTECTED))
                {
                    EXEC_SPINLOCK_LOCK(&mh->mh_SpinLock, NULL, SPINLOCK_MODE_WRITE);
                    res = mhe->mhe_Alloc(mhe, byteSize, &flags);
                    EXEC_SPINLOCK_UNLOCK(&mh->mh_SpinLock);
                }
                else
#endif
                res = mhe->mhe_Alloc(mhe, byteSize, &flags);
            }
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
                    APTR ret;

                    /*
                     * Same rule as nommu_Alloc above: the managed allocator
                     * only serialises itself when MEMF_SEM_PROTECTED, which
                     * the system heap is not. MEM_LOCK alone does not help -
                     * the alloc/free paths hold MemListSpinLock shared only.
                     * SEM_PROTECTED headers may Wait() internally - never
                     * enter them with the spinlock held.
                     */
#if defined(__AROSEXEC_SMP__)
                    if (!((IPTR)mh->mh_First & MEMF_SEM_PROTECTED))
                    {
                        EXEC_SPINLOCK_LOCK(&mh->mh_SpinLock, NULL, SPINLOCK_MODE_WRITE);
                        ret = mhe->mhe_AllocAbs(mhe, byteSize, location);
                        EXEC_SPINLOCK_UNLOCK(&mh->mh_SpinLock);
                    }
                    else
#endif
                    ret = mhe->mhe_AllocAbs(mhe, byteSize, location);

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
            elements from the list we need the node's predecessor. For the
            first element we can use freeList->mh_First instead of a real
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

    /*
     * Shared lock for the list walk; stdDealloc serializes the per-mh write
     * via mh_SpinLock.
     */
    MEM_LOCK_SHARED;

    ForeachNode(&SysBase->MemList, mh)
    {
        if (IsManagedMem(mh))
        {
            struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

            /* Test if the memory belongs to this MemHeader. */
            if (!mhe->mhe_InBounds(mhe, memoryBlock, blockEnd))
                continue;

            if (mhe->mhe_Free)
            {
                /* See nommu_AllocMem: serialise the unprotected managed
                 * heap against concurrent alloc/free on other cores.
                 * SEM_PROTECTED headers serialise themselves and may
                 * Wait() - never enter them with the spinlock held. */
#if defined(__AROSEXEC_SMP__)
                if (!((IPTR)mh->mh_First & MEMF_SEM_PROTECTED))
                {
                    EXEC_SPINLOCK_LOCK(&mh->mh_SpinLock, NULL, SPINLOCK_MODE_WRITE);
                    mhe->mhe_Free(mhe, memoryBlock, byteSize);
                    EXEC_SPINLOCK_UNLOCK(&mh->mh_SpinLock);
                }
                else
#endif
                mhe->mhe_Free(mhe, memoryBlock, byteSize);
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
    bug("[MM] %s() called from 0x%p by '%s'\n",
        loc ? loc->function : "?", loc ? loc->caller : NULL,
        FindTask(NULL)->tc_Node.ln_Name);

    Alert(AN_BadFreeAddr);
#endif

    ReturnVoid ("nommu_FreeMem");
}

IPTR nommu_AvailMem(ULONG attributes, struct ExecBase *SysBase)
{
    IPTR ret = 0;
    struct MemHeader *mh;
    ULONG physFlags = attributes & MEMF_PHYSICAL_MASK;

    D(bug("[MM] nommu_AvailMem(0x%08X)\n", attributes);)
    D(bug("[MM] physical memory flags: 0x%08X\n", physFlags);)

    /* Nobody else should access the memory lists now. */
    MEM_LOCK_SHARED;

    ForeachNode(&SysBase->MemList, mh)
    {
        D(bug("[MM] Checking MemHeader 0x%p\n", mh);)

        /*
         * The current memheader is OK if there's no bit in the
         * 'physFlags' that isn't set in the 'mh->mh_Attributes'.
         */
        if (physFlags & ~mh->mh_Attributes)
        {
            D(bug("[MM] Skipping (mh_Attributes = 0x%08X\n", mh->mh_Attributes);)
            continue;
        }

#if defined(__AROSEXEC_SMP__)
        /*
         * Read this header's free list / managed state under its spinlock
         * in shared mode (we don't mutate) so a concurrent alloc/free on
         * another core can't change the chunk list mid-walk. This is the
         * reader counterpart to the WRITE-mode lock nommu_AllocMem and
         * stdAlloc take around mutations. SEM_PROTECTED managed headers
         * serialise themselves and may Wait() in mhe_Avail - do not hold
         * the spinlock around them.
         */
        BOOL lockmh = !(IsManagedMem(mh) &&
                        ((IPTR)mh->mh_First & MEMF_SEM_PROTECTED));

        if (lockmh)
            EXEC_SPINLOCK_LOCK(&mh->mh_SpinLock, NULL, SPINLOCK_MODE_READ);
#endif

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

#if defined(__AROSEXEC_SMP__)
                if (lockmh)
                    EXEC_SPINLOCK_UNLOCK(&mh->mh_SpinLock);
#endif
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
                    bug("[MM] Chunk allocator error in MemHeader 0x%p\n", mh);
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

#if defined(__AROSEXEC_SMP__)
        if (lockmh)
            EXEC_SPINLOCK_UNLOCK(&mh->mh_SpinLock);
#endif
    }

    /* All done */
    MEM_UNLOCK;

    return ret;
}
