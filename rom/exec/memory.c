/*
    Copyright � 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <exec/rawfmt.h>
#include <exec/memheaderext.h>
#include <proto/kernel.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "etask.h"
#include "memory.h"
#include "mungwall.h"

#define DMH(x)

/*
 * Find MemHeader to which address belongs.
 * This function is legal to be called in supervisor mode (we use TypeOfMem()
 * in order to validate addresses in tons of places). So, here are checks.
 */
struct MemHeader *FindMem(APTR address, struct ExecBase *SysBase)
{
    int usermode = (KernelBase != NULL) && (KrnIsSuper() == 0);
    struct MemHeader *mh;

    /* Nobody should change the memory list now. */
    if (usermode) MEM_LOCK_SHARED;

    /* Follow the list of MemHeaders */
    mh = (struct MemHeader *)SysBase->MemList.lh_Head;

    while (mh->mh_Node.ln_Succ != NULL)
    {
        if (mh->mh_Attributes & MEMF_MANAGED)
        {
            struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

            if (mhe->mhe_InBounds)
            {
                if (mhe->mhe_InBounds(mhe, address, address))
                {
                    if (usermode) MEM_UNLOCK;
                    return mh;
                }
            }
        }
        else
        {
            /* Check if this MemHeader fits */
            if (address >= mh->mh_Lower && address < mh->mh_Upper)
            {
                /* Yes. Return it. */
                if (usermode) MEM_UNLOCK;
                return mh;
            }
        }
        /* Go to next MemHeader */
        mh = (struct MemHeader *)mh->mh_Node.ln_Succ;
    }

    if (usermode) MEM_UNLOCK;
    return NULL;
}

char *FormatMMContext(char *buffer, struct MMContext *ctx, struct ExecBase *SysBase)
{
    if (ctx->addr)
        buffer = NewRawDoFmt("In %s, block at 0x%p, size %lu", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->func, ctx->addr, ctx->size) - 1;
    else
        buffer = NewRawDoFmt("In %s, size %lu", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->func, ctx->size) - 1;
    
    if (ctx->mc)
    {
        buffer = NewRawDoFmt("\nCorrupted MemChunk 0x%p (next 0x%p, size %lu)", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->mc, ctx->mc->mc_Next, ctx->mc->mc_Bytes) - 1;
        
        if (ctx->mcPrev)
            buffer = NewRawDoFmt("\nPrevious MemChunk 0x%p (next 0x%p, size %lu)", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->mcPrev, ctx->mcPrev->mc_Next, ctx->mcPrev->mc_Bytes) - 1;
    }

    /* Print MemHeader details */
    buffer = NewRawDoFmt("\nMemHeader 0x%p (0x%p - 0x%p)", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->mh, ctx->mh->mh_Lower, ctx->mh->mh_Upper) - 1;
    if ((IPTR)ctx->mh->mh_First & (MEMCHUNK_TOTAL - 1))
        buffer = NewRawDoFmt("\n- Unaligned first chunk address (0x%p)", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->mh->mh_First) - 1;

    if (ctx->mh->mh_Free & (MEMCHUNK_TOTAL - 1))
        buffer = NewRawDoFmt("\n- Unaligned free space count (0x%p)", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->mh->mh_Free) - 1;

    if (ctx->mh->mh_First)
    {
        if ((APTR)ctx->mh->mh_First < ctx->mh->mh_Lower)
            buffer = NewRawDoFmt("\n- First chunk (0x%p) below lower address", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->mh->mh_First) - 1;

        if (((APTR)ctx->mh->mh_First + ctx->mh->mh_Free > ctx->mh->mh_Upper))
            buffer = NewRawDoFmt("\n- Free space count too large (%lu, first chunk 0x%xp)", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->mh->mh_Free, ctx->mh->mh_First) - 1;
    }

    return buffer;
}

/* #define NO_ALLOCATOR_CONTEXT */

#ifdef NO_ALLOCATOR_CONTEXT

struct MemHeaderAllocatorCtx * mhac_GetSysCtx(struct MemHeader * mh, struct ExecBase * SysBase)
{
    return NULL;
}

void mhac_PoolMemHeaderSetup(struct MemHeader * mh, struct ProtectedPool * pool)
{
    mh->mh_Node.ln_Name = (STRPTR)pool;
}

ULONG mhac_GetCtxSize()
{
    return 0;
}

void mhac_ClearSysCtx(struct MemHeader * mh, struct ExecBase * SysBase)
{
}

#define mhac_MemChunkClaimed(a, b)
#define mhac_IsIndexEmpty(a)                (TRUE)
#define mhac_ClearIndex(a)
#define mhac_MemChunkCreated(a, b, c)       { (void)b; }
#define mhac_GetBetterPrevMemChunk(a, b, c) (a)
#define mhac_GetCloserPrevMemChunk(a, b, c) (a)
#define mhac_PoolMemHeaderGetCtx(a)         (NULL)
#define mhac_PoolMemHeaderGetPool(a)        (a->mh_Node.ln_Name)

#else
/* Allocator optimization support */

/*
 * The array contains pointers to chunk previous to first chunk of at least size N
 *
 * N = 1 << (FIRSTPOTBIT + (i * POTSTEP)), where i is index in array
 * first is defined as MemChunk with lowest address
 *
 * Each chunk in array locates the place where search should start, not necesarly
 * where allocation should happen.
 *
 * If chunk is taken from MemHeader and is present in the index, it must be removed
 * from index.
 *
 * If chunk is returned to MemHeader it may be registered with index.
 */

#define FIRSTPOTBIT             (5)
#define FIRSTPOT                (1 << FIRSTPOTBIT)
#define POTSTEP                 (1)     /* Distance between each level */
#define ALLOCATORCTXINDEXSIZE   (10)    /* Number of levels in index */

struct MemHeaderAllocatorCtx
{
    struct Node             mhac_Node;
    struct MemHeader        *mhac_MemHeader;
    APTR                    mhac_Data1;

    ULONG                   mhac_IndexSize;
    struct MemChunk         *mhac_PrevChunks[ALLOCATORCTXINDEXSIZE];
};

ULONG mhac_GetCtxSize()
{
    return (AROS_ROUNDUP2(sizeof(struct MemHeaderAllocatorCtx), MEMCHUNK_TOTAL));
}

static BOOL mhac_IsIndexEmpty(struct MemHeaderAllocatorCtx * mhac)
{
    LONG i;
    if (!mhac)
        return TRUE;

    for (i = 0; i < mhac->mhac_IndexSize; i++)
        if (mhac->mhac_PrevChunks[i] != NULL)
            return FALSE;

    return TRUE;
}

static void mhac_ClearIndex(struct MemHeaderAllocatorCtx * mhac)
{
    LONG i;

    if (!mhac)
        return;

    for (i = 0; i < ALLOCATORCTXINDEXSIZE; i++)
        mhac->mhac_PrevChunks[i] = NULL;
}

static void mhac_SetupMemHeaderAllocatorCtx(struct MemHeader * mh, ULONG maxindexsize,
        struct MemHeaderAllocatorCtx * mhac)
{
    /* Adjust index size to space in MemHeader */
    IPTR size = (IPTR)mh->mh_Upper - (IPTR)mh->mh_Lower;
    LONG indexsize = 0;

    size = size >> FIRSTPOTBIT;
    size = size >> POTSTEP;

    for (; size > 0; size = size >> POTSTEP) indexsize++;

    if (indexsize < 0) indexsize = 0;
    if (indexsize > maxindexsize) indexsize = maxindexsize;
    if (indexsize > ALLOCATORCTXINDEXSIZE) indexsize = ALLOCATORCTXINDEXSIZE;

    mhac->mhac_MemHeader = mh;
    mhac->mhac_IndexSize = indexsize;
    mhac_ClearIndex(mhac);
}

void mhac_ClearSysCtx(struct MemHeader * mh, struct ExecBase * SysBase)
{
    struct MemHeaderAllocatorCtx * mhac = NULL;

    ForeachNode(&PrivExecBase(SysBase)->AllocatorCtxList, mhac)
    {
        if (mhac->mhac_MemHeader == mh)
        {
            mhac_ClearIndex(mhac);
            break;
        }
    }
}

struct MemHeaderAllocatorCtx * mhac_GetSysCtx(struct MemHeader * mh, struct ExecBase * SysBase)
{
    struct MemHeaderAllocatorCtx * mhac = NULL;

    ForeachNode(&PrivExecBase(SysBase)->AllocatorCtxList, mhac)
    {
        if (mhac->mhac_MemHeader == mh)
            return mhac;
    }

    /* New context is needed */
    mhac = Allocate(mh, sizeof(struct MemHeaderAllocatorCtx));
    mhac_SetupMemHeaderAllocatorCtx(mh, ALLOCATORCTXINDEXSIZE, mhac);
    AddTail(&PrivExecBase(SysBase)->AllocatorCtxList, (struct Node *)mhac);

    return mhac;
}

static void mhac_MemChunkClaimed(struct MemChunk * mc, struct MemHeaderAllocatorCtx * mhac)
{
    LONG i;

    if (!mhac)
        return;

    for (i = 0; i < mhac->mhac_IndexSize; i++)
    {
        if (mhac->mhac_PrevChunks[i] != NULL &&
                (mhac->mhac_PrevChunks[i] == mc || mhac->mhac_PrevChunks[i]->mc_Next == mc))
        {
            mhac->mhac_PrevChunks[i] = NULL;
        }
    }
}

static LONG mhac_CalcIndex(LONG size, ULONG indexsize)
{
    LONG r = 0;
    size >>= FIRSTPOTBIT;
    while (size >>= 1)
        r++;

    if (r > indexsize - 1) r = indexsize - 1;

    return r;
}

static void mhac_MemChunkCreated(struct MemChunk * mc, struct MemChunk *mcprev, struct MemHeaderAllocatorCtx * mhac)
{
    LONG i, v = FIRSTPOT;

    if (mc->mc_Bytes < FIRSTPOT) /* Allocation too small for index */
        return;

    if (!mhac)
        return;

    for (i = 0; i < mhac->mhac_IndexSize; i++, v = v << POTSTEP)
    {
        if (mc->mc_Bytes < v)
            break; /* Chunk smaller than index at i. Stop */

        /* If no chunk in index or given passed chunk has lower address than chunk in index */
        if (mhac->mhac_PrevChunks[i] == NULL ||
                (mhac->mhac_PrevChunks[i] != NULL && mhac->mhac_PrevChunks[i]->mc_Next > mc))
        {
            mhac->mhac_PrevChunks[i] = mcprev;
        }
    }
}

/* General idea:
 *      Function returned pointer to chunk that is prev to chunk that will allow
 *      to locate faster chunk big enough for allocation. Function never returns NULL.
 * Current implementation:
 *      Function returns pointer to chunk that is prev to first biggest chunk,
 *      not bigger than requested size
 */
static struct MemChunk * mhac_GetBetterPrevMemChunk(struct MemChunk * prev, IPTR size, struct MemHeaderAllocatorCtx * mhac)
{
    struct MemChunk * _return = prev;

    if (size < FIRSTPOT)
        return _return; /* Allocation too small for index */

    if (mhac)
    {
        LONG i;
        LONG ii = mhac_CalcIndex(size, mhac->mhac_IndexSize);

        if (mhac->mhac_PrevChunks[ii] != NULL)
            _return = mhac->mhac_PrevChunks[ii];
        else
        {
            for (i = ii - 1; i >= 0; i--)
            {
                if (mhac->mhac_PrevChunks[i] != NULL)
                {
                    _return = mhac->mhac_PrevChunks[i];
                    break;
                }
            }
        }
    }

    return _return;
}

static struct MemChunk * mhac_GetCloserPrevMemChunk(struct MemChunk * prev, APTR addr, struct MemHeaderAllocatorCtx * mhac)
{
    struct MemChunk * _return = prev;

    if (mhac)
    {
        LONG i;

        for (i = 0; i < mhac->mhac_IndexSize; i++)
        {
            if (mhac->mhac_PrevChunks[i] != NULL &&
                    (APTR)mhac->mhac_PrevChunks[i]->mc_Next < addr &&
                    mhac->mhac_PrevChunks[i]->mc_Next > _return->mc_Next)
            {
                _return = mhac->mhac_PrevChunks[i];
            }
        }
    }

    return _return;
}

/*
 * Enhace MemHeader that is part of pool with MemHeaderAllocatorContext
 */
void mhac_PoolMemHeaderSetup(struct MemHeader * mh, struct ProtectedPool * pool)
{
    struct MemHeaderAllocatorCtx * mhac = Allocate(mh, sizeof(struct MemHeaderAllocatorCtx));

    mhac_SetupMemHeaderAllocatorCtx(mh, 5, mhac);

    mhac->mhac_Data1 = pool;
    mh->mh_Node.ln_Name = (STRPTR)mhac;
}

#define mhac_PoolMemHeaderGetCtx(a)     ((struct MemHeaderAllocatorCtx *)(a->mh_Node.ln_Name))
#define mhac_PoolMemHeaderGetPool(a)    (mhac_PoolMemHeaderGetCtx(a)->mhac_Data1)

#endif


#ifdef NO_CONSISTENCY_CHECKS

#define validateHeader(mh, op, addr, size, SysBase) TRUE
#define validateChunk(mc, prev, mh, op, addr, size, SysBase) TRUE

#else

static ULONG memAlerts[] =
{
    AT_DeadEnd|AN_MemoryInsane, /* MM_ALLOC   */
    AT_DeadEnd|AN_MemCorrupt,   /* MM_FREE    */
    AN_FreeTwice                /* MM_OVERLAP */
};

/*
 * MemHeader validation routine. Rules are:
 *
 * 1. Both mh_First and mh_Free must be MEMCHUNK_TOTAL-aligned.
 * 2. Free space (if present) must completely fit in between mh_Lower and mh_Upper.
 * We intentionally don't check header's own location. We assume that in future we'll
 * be able to put MEMF_CHIP headers inside MEMF_FAST memory, for speed up.
 */
static BOOL validateHeader(struct MemHeader *mh, UBYTE op, APTR addr, IPTR size, struct TraceLocation *tp, struct ExecBase *SysBase)
{
    if (((IPTR)mh->mh_First & (MEMCHUNK_TOTAL - 1)) || (mh->mh_Free & (MEMCHUNK_TOTAL - 1)) ||        /* 1 */
    (mh->mh_First &&
     (((APTR)mh->mh_First < mh->mh_Lower) || ((APTR)mh->mh_First + mh->mh_Free > mh->mh_Upper))))    /* 2 */
    {
        if (tp)
        {
            /* TraceLocation is not supplied by PrepareExecBase(). Fail silently. */
            struct MMContext alertData;

            alertData.mh     = mh;
            alertData.mc     = NULL;
            alertData.mcPrev = NULL;
            alertData.func   = tp->function;
            alertData.addr   = addr;
            alertData.size   = size;
            alertData.op     = op;

            Exec_ExtAlert(memAlerts[op], tp->caller, tp->stack, AT_MEMORY, &alertData, SysBase);
        }

        /*
         * Theoretically during very early boot we can fail to post an alert (no KernelBase yet).
         * In this case we return with fault indication.
         */
        return FALSE;
    }
    return TRUE;
}

/*
 * MemChunk consistency check. Rules are:
 *
 * 1. Both mc_Next and mc_Bytes must me MEMCHUNK_TOTAL-aligned, and mc_Bytes can not be zero.
 * 2. End of this chunk must not be greater than mh->mh_Upper
 * 3. mc_Next (if present) must point in between end of this chunk and mh->mh_Upper - MEMCHUNK_TOTAL.
 *    There must be at least MEMHCUNK_TOTAL allocated bytes between free chunks.
 *
 * This function is inlined for speed improvements.
 */
static inline BOOL validateChunk(struct MemChunk *p2, struct MemChunk *p1, struct MemHeader *mh,
                 UBYTE op, APTR addr, IPTR size,
                 struct TraceLocation *tp, struct ExecBase *SysBase)
{
    if (((IPTR)p2->mc_Next & (MEMCHUNK_TOTAL-1)) || (p2->mc_Bytes == 0) || (p2->mc_Bytes & (MEMCHUNK_TOTAL-1))    ||    /* 1 */
    ((APTR)p2 + p2->mc_Bytes > mh->mh_Upper)                                ||    /* 2 */
    (p2->mc_Next && (((APTR)p2->mc_Next < (APTR)p2 + p2->mc_Bytes + MEMCHUNK_TOTAL) ||                /* 3 */
                 ((APTR)p2->mc_Next > mh->mh_Upper - MEMCHUNK_TOTAL))))
    {
        if (tp)
        {
            struct MMContext alertData;

            alertData.mh     = mh;
            alertData.mc     = p2;
            alertData.mcPrev = (p1 == (struct MemChunk *)&mh->mh_First) ? NULL : p1;
            alertData.func   = tp->function;
            alertData.addr   = addr;
            alertData.size   = size;
            alertData.op     = op;

            Exec_ExtAlert(memAlerts[op], tp->caller, tp->stack, AT_MEMORY, &alertData, SysBase);
        }
        return FALSE;
    }

    return TRUE;
}

#endif

/*
 * Allocate block from the given MemHeader in a specific way.
 * This routine can be called with SysBase = NULL.
 * MemHeaderAllocatorCtx
 *      This parameter is optional, allocation needs to work without it as well.
 *      However if it was passed once for a given MemHeader it needs to be passed
 *      in all consecutive calls.
 */
APTR stdAlloc(struct MemHeader *mh, struct MemHeaderAllocatorCtx *mhac, IPTR size,
        ULONG requirements, struct TraceLocation *tp, struct ExecBase *SysBase)
{
    /*
     * The check has to be done for the second time. Exec uses stdAlloc on memheader
     * passed upon startup. This is bad, very bad. So here a temporary hack :)
     */
    if ((mh->mh_Node.ln_Type == NT_MEMORY) &&
        (mh->mh_Attributes & MEMF_MANAGED) &&
        (((struct MemHeaderExt *)mh)->mhe_Magic == MEMHEADER_EXT_MAGIC))
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

        if (mhe->mhe_Alloc)
        {
            return mhe->mhe_Alloc(mhe, size, &requirements);
        }
        else
            return NULL;
    }
    else
    {
        /* First round byteSize up to a multiple of MEMCHUNK_TOTAL */
        IPTR byteSize = AROS_ROUNDUP2(size, MEMCHUNK_TOTAL);
        struct MemChunk *mc=NULL, *p1, *p2;

        /* Validate MemHeader before doing anything. */
        if (!validateHeader(mh, MM_ALLOC, NULL, size, tp, SysBase))
            return NULL;

        /* Validate if there is even enough total free memory */
        if (mh->mh_Free < byteSize)
            return NULL;


        /*
         * The free memory list is only single linked, i.e. to remove
         * elements from the list I need the node's predecessor. For the
         * first element I can use mh->mh_First instead of a real predecessor.
         */
        p1 = mhac_GetBetterPrevMemChunk((struct MemChunk *)&mh->mh_First, size, mhac);
        p2 = p1->mc_Next;

        /*
         * Follow the memory list. p1 is the previous MemChunk, p2 is the current one.
         * On 1st pass p1 points to mh->mh_First, so that changing p1->mc_Next actually
         * changes mh->mh_First.
         */
        while (p2 != NULL)
        {
            /* Validate the current chunk */
            if (!validateChunk(p2, p1, mh, MM_ALLOC, NULL, size, tp, SysBase))
                return NULL;

            /* Check if the current block is large enough */
            if (p2->mc_Bytes>=byteSize)
            {
                /* It is. */
                mc = p1;

                /* Use this one if MEMF_REVERSE is not set.*/
                if (!(requirements & MEMF_REVERSE))
                    break;
                /* Else continue - there may be more to come. */
            }

            /* Go to next block */
            p1 = p2;
            p2 = p1->mc_Next;
        }

        /* Something found? */
        if (mc != NULL)
        {
            /* Remember: if MEMF_REVERSE is set p1 and p2 are now invalid. */
            p1 = mc;
            p2 = p1->mc_Next;

            mhac_MemChunkClaimed(p2, mhac);

            /* Remove the block from the list and return it. */
            if (p2->mc_Bytes == byteSize)
            {
                /* Fits exactly. Just relink the list. */
                p1->mc_Next = p2->mc_Next;
                mc          = p2;
            }
            else
            {
                struct MemChunk * pp = p1;

                if (requirements & MEMF_REVERSE)
                {
                    /* Return the last bytes. */
                    p1->mc_Next=p2;
                    mc = (struct MemChunk *)((UBYTE *)p2+p2->mc_Bytes-byteSize);
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

                mhac_MemChunkCreated(p1, pp, mhac);
            }

            mh->mh_Free -= byteSize;

            /* Clear the block if requested */
            if (requirements & MEMF_CLEAR)
                memset(mc, 0, byteSize);
        }
        else
        {
            if (!mhac_IsIndexEmpty(mhac))
            {
                /*
                 * Since chunks created during deallocation are not returned to index,
                 * retry with cleared index.
                 */
                mhac_ClearIndex(mhac);
                mc = stdAlloc(mh, mhac, size, requirements, tp, SysBase);
            }
        }

        return mc;
    }
}

/*
 * Free 'byteSize' bytes starting at 'memoryBlock' belonging to MemHeader 'freeList'
 * MemHeaderAllocatorCtx
 *      See stdAlloc
 */
void stdDealloc(struct MemHeader *freeList, struct MemHeaderAllocatorCtx *mhac, APTR addr, IPTR size, struct TraceLocation *tp, struct ExecBase *SysBase)
{
    APTR memoryBlock;
    IPTR byteSize;
    struct MemChunk *p1, *p2, *p3;
    UBYTE *p4;

    if ((freeList->mh_Node.ln_Type == NT_MEMORY) &&
        (freeList->mh_Attributes & MEMF_MANAGED) &&
        (((struct MemHeaderExt *)freeList)->mhe_Magic == MEMHEADER_EXT_MAGIC))
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)freeList;
        
        if (mhe->mhe_Free)
            mhe->mhe_Free(mhe, addr, size);
    }
    else
    {
        /* Make sure the MemHeader is OK */
        if (!validateHeader(freeList, MM_FREE, addr, size, tp, SysBase))
            return;

        /* Align size to the requirements */
        byteSize = size + ((IPTR)addr & (MEMCHUNK_TOTAL - 1));
        byteSize = (byteSize + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL - 1);

        /* Align the block as well */
        memoryBlock = (APTR)((IPTR)addr & ~(MEMCHUNK_TOTAL-1));

        /*
        The free memory list is only single linked, i.e. to insert
        elements into the list I need the node as well as its
        predecessor. For the first element I can use freeList->mh_First
        instead of a real predecessor.
        */
        p1 = (struct MemChunk *)&freeList->mh_First;
        p2 = freeList->mh_First;

        /* Start and end(+1) of the block */
        p3 = (struct MemChunk *)memoryBlock;
        p4 = (UBYTE *)p3 + byteSize;

        /* No chunk in list? Just insert the current one and return. */
        if (p2 == NULL)
        {
            p3->mc_Bytes = byteSize;
            p3->mc_Next = NULL;
            p1->mc_Next = p3;
            freeList->mh_Free += byteSize;
            return;
        }

        /* Find closer chunk */
        p1=mhac_GetCloserPrevMemChunk(p1, addr, mhac);
        p2=p1->mc_Next;

        /* Follow the list to find a place where to insert our memory. */
        do
        {
            if (!validateChunk(p2, p1, freeList, MM_FREE, addr, size, tp, SysBase))
                return;

            /* Found a block with a higher address? */
            if (p2 >= p3)
            {
#if !defined(NO_CONSISTENCY_CHECKS)
                /*
                If the memory to be freed overlaps with the current
                block something must be wrong.
                */
                if (p4>(UBYTE *)p2)
                {
                    bug("[MM] Chunk allocator error\n");
                    bug("[MM] Attempt to free %u bytes at 0x%p from MemHeader 0x%p\n", byteSize, memoryBlock, freeList);
                    bug("[MM] Block overlaps (1) with chunk 0x%p (%u bytes)\n", p2, p2->mc_Bytes);

                    Alert(AN_FreeTwice);
                    return;
                }
#endif
                /* End the loop with p2 non-zero */
                break;
            }
            /* goto next block */
            p1 = p2;
            p2 = p2->mc_Next;

            /* If the loop ends with p2 zero add it at the end. */
        } while (p2 != NULL);

        /* If there was a previous block merge with it. */
        if (p1 != (struct MemChunk *)&freeList->mh_First)
        {
#if !defined(NO_CONSISTENCY_CHECKS)
            /* Check if they overlap. */
            if ((UBYTE *)p1 + p1->mc_Bytes > (UBYTE *)p3)
            {
                bug("[MM] Chunk allocator error\n");
                bug("[MM] Attempt to free %u bytes at 0x%p from MemHeader 0x%p\n", byteSize, memoryBlock, freeList);
                bug("[MM] Block overlaps (2) with chunk 0x%p (%u bytes)\n", p1, p1->mc_Bytes);

                Alert(AN_FreeTwice);
                return;
            }
#endif
            /* Merge if possible */
            if ((UBYTE *)p1 + p1->mc_Bytes == (UBYTE *)p3)
            {
                mhac_MemChunkClaimed(p1, mhac);
                p3 = p1;
                /*
                 * Note: this case does not lead to mhac_MemChunkCreated, because
                 * we don't have chunk prev to p1
                 */
            }
            else
                /* Not possible to merge */
                p1->mc_Next = p3;
        }else
            /*
                There was no previous block. Just insert the memory at
                the start of the list.
            */
            p1->mc_Next = p3;

        /* Try to merge with next block (if there is one ;-) ). */
        if (p4 == (UBYTE *)p2 && p2 != NULL)
        {
            /*
               Overlap checking already done. Doing it here after
               the list potentially changed would be a bad idea.
            */
            mhac_MemChunkClaimed(p2, mhac);
            p4 += p2->mc_Bytes;
            p2 = p2->mc_Next;
        }
        /* relink the list and return. */
        p3->mc_Next = p2;
        p3->mc_Bytes = p4 - (UBYTE *)p3;
        freeList->mh_Free += byteSize;
        if (p1->mc_Next==p3) mhac_MemChunkCreated(p3, p1, mhac);
    }
}

/* 
 * TODO:
 * During transition period four routines below use nommu allocator.
 * When transition is complete they should use them only if MMU
 * is inactive. Otherwise they should use KrnAllocPages()/KrnFreePages().
 */

/* Non-mungwalled AllocAbs(). Does not destroy sideways regions. */
APTR InternalAllocAbs(APTR location, IPTR byteSize, struct ExecBase *SysBase)
{
    return nommu_AllocAbs(location, byteSize, SysBase);
}

/*
 * Use this if you want to free region allocated by InternalAllocAbs().
 * Otherwise you hit mungwall problem (FreeMem() expects header).
 */
void InternalFreeMem(APTR location, IPTR byteSize, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    nommu_FreeMem(location, byteSize, loc, SysBase);
}

/*
 * Allocate a region managed by own header. Usable size is reduced by size
 * of header.
 */
APTR AllocMemHeader(IPTR size, ULONG flags, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    struct MemHeader *mh;

    mh = nommu_AllocMem(size, flags, loc, SysBase);
    DMH(bug("[AllocMemHeader] Allocated %u bytes at 0x%p\n", size, mh));

    if (mh)
    {
        struct MemHeader *orig = FindMem(mh, SysBase);

        if (orig->mh_Attributes & MEMF_MANAGED)
        {
            struct MemHeaderExt *mhe_orig = (struct MemHeaderExt *)orig;
            struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;
            IPTR header_size = (sizeof(struct MemHeaderExt) + 15) & ~15;

            /* Copy the basic information */
            mh->mh_Node.ln_Type     = NT_MEMORY;
            mh->mh_Node.ln_Pri      = orig->mh_Node.ln_Pri;
            mh->mh_Attributes       = orig->mh_Attributes;
            mh->mh_Upper            = (void *)mh + size;
            mh->mh_Lower            = (void *)mh;
            mh->mh_First            = (APTR)(IPTR)flags;
            mh->mh_Free             = 0;

            mhe->mhe_Magic          = mhe_orig->mhe_Magic;

            /* Copy init functions */
            mhe->mhe_InitPool       = mhe_orig->mhe_InitPool;
            mhe->mhe_DestroyPool    = mhe_orig->mhe_DestroyPool;

            /* Copy memory allocation functions */
            mhe->mhe_Alloc          = mhe_orig->mhe_Alloc;
            mhe->mhe_AllocAbs       = mhe_orig->mhe_AllocAbs;
            mhe->mhe_AllocVec       = mhe_orig->mhe_AllocVec;
            mhe->mhe_Avail          = mhe_orig->mhe_Avail;
            mhe->mhe_Free           = mhe_orig->mhe_Free;
            mhe->mhe_FreeVec        = mhe_orig->mhe_FreeVec;
            mhe->mhe_InBounds       = mhe_orig->mhe_InBounds;
            mhe->mhe_ReAlloc        = mhe_orig->mhe_ReAlloc;

            /*
             * User data will be initialized. Memory pool will get first region
             * for free.
             */
            mhe->mhe_UserData       = (APTR)mh + header_size;

            /* Initialize the pool with rest size */
            if (mhe->mhe_InitPool)
                mhe->mhe_InitPool(mhe, size, size - header_size);
        }
        else
        {
            size -= MEMHEADER_TOTAL;

            /*
             * Initialize new MemHeader.
             * Inherit attributes from system MemHeader from which
             * our chunk was allocated.
             */
            mh->mh_Node.ln_Type     = NT_MEMORY;
            mh->mh_Node.ln_Pri      = orig->mh_Node.ln_Pri;
            mh->mh_Attributes       = orig->mh_Attributes;
            mh->mh_Lower            = (APTR)mh + MEMHEADER_TOTAL;
            mh->mh_Upper            = mh->mh_Lower + size;
            mh->mh_First            = mh->mh_Lower;
            mh->mh_Free             = size;

            /* Create the first (and the only) MemChunk */
            mh->mh_First->mc_Next   = NULL;
            mh->mh_First->mc_Bytes  = size;
        }
    }
    return mh;
}

/* Free a region allocated by AllocMemHeader() */
void FreeMemHeader(APTR addr, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    struct MemHeaderExt *mhe = (struct MemHeaderExt *)addr;

    IPTR size = (IPTR)mhe->mhe_MemHeader.mh_Upper - (IPTR)addr;

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_MANAGED)
    {
        if (mhe->mhe_DestroyPool)
            mhe->mhe_DestroyPool(mhe);
    }

    DMH(bug("[FreeMemHeader] Freeing %u bytes at 0x%p\n", size, addr));
    nommu_FreeMem(addr, size, loc, SysBase);
}

/*
 * This is our own Enqueue() version. Currently the only differece is that
 * we insert our node before the first node with LOWER OR EQUAL priority,
 * so that for nodes with equal priority it will be LIFO, not FIFO queue.
 * This speeds up the allocator.
 * TODO: implement secondary sorting by mh_Free. This will allow to
 * implement best-match algorithm (so that puddles with smaller free space
 * will be picked up first). This way the smallest allocations will reuse
 * smallest chunks instead of fragmenting large ones.
 */
static void EnqueueMemHeader(struct MinList *list, struct MemHeader *mh)
{
    struct MemHeader *next;

    /* Look through the list */
    ForeachNode (list, next)
    {
        /*
            Look for the first MemHeader with a lower or equal pri as the node
            we have to insert into the list.
        */
        if (mh->mh_Node.ln_Pri >= next->mh_Node.ln_Pri)
            break;
    }

    /* Insert the node before next */
    mh->mh_Node.ln_Pred           = next->mh_Node.ln_Pred;
    mh->mh_Node.ln_Succ           = &next->mh_Node;
    next->mh_Node.ln_Pred->ln_Succ = &mh->mh_Node;
    next->mh_Node.ln_Pred          = &mh->mh_Node;
}

/*
 * Allocate memory with given physical properties from the given pool.
 * Our pools can be mixed. This means that different puddles from the
 * pool can have different physical flags. For example the same pool
 * can contain puddles from both CHIP and FAST memory. This is done in
 * order to provide a single system default pool for all types of memory.
 */
APTR InternalAllocPooled(APTR poolHeader, IPTR memSize, ULONG flags, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    struct ProtectedPool *pool = poolHeader + MEMHEADER_TOTAL;
    APTR ret = NULL;
    IPTR origSize;
    struct MemHeader *mh;

    D(bug("[exec] InternalAllocPooled(0x%p, %u, 0x%08X), header 0x%p\n", poolHeader, memSize, flags, pool));

    /*
     * Memory blocks allocated from the pool store pointers to the MemHeader they were
     * allocated from. This is done in order to avoid slow lookups in InternalFreePooled().
     * This is done in AllocVec()-alike manner; the pointer is placed right before the block.
     */
    memSize += sizeof(struct MemHeader *);
    origSize = memSize;

    /* If mungwall is enabled, count also size of walls */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
        memSize += MUNGWALL_TOTAL_SIZE;

    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
        ObtainSemaphore(&pool->sem);
    }

    /* Follow the list of MemHeaders */
    mh = (struct MemHeader *)pool->pool.PuddleList.mlh_Head;
    for(;;)
    {
        ULONG physFlags = flags & MEMF_PHYSICAL_MASK;

        /* Are there no more MemHeaders? */
        if (mh->mh_Node.ln_Succ == NULL)
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
             * Our final puddle size still includes MEMHEADER_TOTAL +
             * allocator ctx size in any case.
             */
            IPTR puddleSize = pool->pool.PuddleSize;

            if (memSize > puddleSize - (MEMHEADER_TOTAL + mhac_GetCtxSize()))
            {
                IPTR align = PrivExecBase(SysBase)->PageSize - 1;

                puddleSize = memSize + MEMHEADER_TOTAL + mhac_GetCtxSize();
                /* Align the size up to page boundary */
                puddleSize = (puddleSize + align) & ~align;
            }

            mh = AllocMemHeader(puddleSize, flags, loc, SysBase);
            D(bug("[InternalAllocPooled] Allocated new puddle 0x%p, size %u\n", mh, puddleSize));

            /* No memory left? */
            if (mh == NULL)
                break;

            /* Add the new puddle to our pool */
            mhac_PoolMemHeaderSetup(mh, pool);
            Enqueue((struct List *)&pool->pool.PuddleList, &mh->mh_Node);

            /* Fall through to get the memory */
        }
        else
        {
            /* Ignore existing MemHeaders with free memory smaller than allocation */
            if (mh->mh_Free < memSize)
            {
                mh = (struct MemHeader *)mh->mh_Node.ln_Succ;
                continue;
            }


            /* Ignore existing MemHeaders with memory type that differ from the requested ones */
            if (physFlags & ~mh->mh_Attributes)
            {
                D(bug("[InternalAllocPooled] Wrong flags for puddle 0x%p (wanted 0x%08X, have 0x%08X\n", flags, mh->mh_Attributes));

                mh = (struct MemHeader *)mh->mh_Node.ln_Succ;
                continue;
            }
        }

        /* Try to get the memory */
        ret = stdAlloc(mh, mhac_PoolMemHeaderGetCtx(mh), memSize, flags, loc, SysBase);
        D(bug("[InternalAllocPooled] Allocated memory at 0x%p from puddle 0x%p\n", ret, mh));

        /* Got it? */
        if (ret != NULL)
        {
            /*
             * If this is not the first MemHeader and it has some free space,
             * move it forward (so that the next allocation will attempt to use it first).
             * IMPORTANT: We use modification of Enqueue() because we still sort MemHeaders
             * according to their priority (which they inherit from system MemHeaders).
             * This allows us to have mixed pools (e.g. with both CHIP and FAST regions). This
             * will be needed in future for memory protection.
             */
            if (mh->mh_Node.ln_Pred != NULL && mh->mh_Free > 32)
            {
                D(bug("[InternalAllocPooled] Re-sorting puddle list\n"));
                Remove(&mh->mh_Node);
                EnqueueMemHeader(&pool->pool.PuddleList, mh);
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
        /* Build munge walls if requested */
        ret = MungWall_Build(ret, pool, origSize, flags, loc, SysBase);

        /* Remember where we were allocated from */
        *((struct MemHeader **)ret) = mh;
        ret += sizeof(struct MemHeader *);
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
void InternalFreePooled(APTR poolHeader, APTR memory, IPTR memSize, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    struct MemHeader *mh;
    APTR freeStart;
    IPTR freeSize;

    D(bug("[exec] InternalFreePooled(0x%p, %u)\n", memory, memSize));

    if (!memory || !memSize) return;

    /* Get MemHeader pointer. It is stored right before our block. */
    freeStart = memory - sizeof(struct MemHeader *);
    freeSize = memSize + sizeof(struct MemHeader *);
    mh = *((struct MemHeader **)freeStart);

    /* Check walls first */
    freeStart = MungWall_Check(freeStart, freeSize, loc, SysBase);
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
        freeSize += MUNGWALL_TOTAL_SIZE;

    /* Verify that MemHeader pointer is correct */
    if ((mh->mh_Node.ln_Type != NT_MEMORY) ||
    (freeStart < mh->mh_Lower) || (freeStart + freeSize > mh->mh_Upper))
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

        Alert(AN_BadFreeAddr);
    }
    else
    {
        struct ProtectedPool *pool = (struct ProtectedPool *)mhac_PoolMemHeaderGetPool(mh);
        IPTR size;
        APTR poolHeaderMH = (APTR)((IPTR)pool - MEMHEADER_TOTAL);

        if (poolHeaderMH != poolHeader)
        {
            bug("[MM] Pool manager error\n");
            bug("[MM] Attempt to free %u bytes at 0x%p\n", memSize, memory);
            bug("[MM] The chunk belongs to pool 0x%p, but call indicated pool 0x%p\n", poolHeaderMH, poolHeader);
            Alert(AN_BadFreeAddr);
        }

        if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
        {
            ObtainSemaphore(&pool->sem);
        }

        size = mh->mh_Upper - mh->mh_Lower;
        D(bug("[FreePooled] Allocated from puddle 0x%p, size %u\n", mh, size));

        /* Free the memory. */
        stdDealloc(mh, mhac_PoolMemHeaderGetCtx(mh), freeStart, freeSize, loc, SysBase);
        D(bug("[FreePooled] Deallocated chunk, %u free bytes in the puddle\n", mh->mh_Free));

        /* Is this MemHeader completely free now? */
        if (mh->mh_Free == size)
        {
            D(bug("[FreePooled] Puddle is empty, giving back to the system\n"));

            /* Yes. Remove it from the list. */
            Remove(&mh->mh_Node);
            /* And free it. */
            FreeMemHeader(mh, loc, SysBase);
        }
        /* All done. */
    
        if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
        {
            ReleaseSemaphore(&pool->sem);
        }
    }
}

ULONG checkMemHandlers(struct checkMemHandlersState *cmhs, struct ExecBase *SysBase)
{
    struct Node      *tmp;
    struct Interrupt *lmh;

    if (cmhs->cmhs_Data.memh_RequestFlags & MEMF_NO_EXPUNGE)
        return MEM_DID_NOTHING;

    /* In order to keep things clean, we must run in a single thread */
    ObtainSemaphore(&PrivExecBase(SysBase)->LowMemSem);

    /*
     * Loop over low memory handlers. Handlers can remove
     * themselves from the list while being invoked, thus
     * we need to be careful!
     */
    for (lmh = (struct Interrupt *)cmhs->cmhs_CurNode;
         (tmp = lmh->is_Node.ln_Succ);
         lmh = (struct Interrupt *)(cmhs->cmhs_CurNode = tmp))
    {
        ULONG ret;

        ret = AROS_UFC3 (LONG, lmh->is_Code,
                   AROS_UFCA(struct MemHandlerData *, &cmhs->cmhs_Data, A0),
                   AROS_UFCA(APTR,                     lmh->is_Data,    A1),
                   AROS_UFCA(struct ExecBase *,        SysBase,         A6)
              );

        if (ret == MEM_TRY_AGAIN)
        {
            /* MemHandler said he did something. Try again. */
            /* Is there any program that depends on this flag??? */
            cmhs->cmhs_Data.memh_Flags |= MEMHF_RECYCLE;
            
            ReleaseSemaphore(&PrivExecBase(SysBase)->LowMemSem);
            return MEM_TRY_AGAIN;
        }
        else
        {
            /* Nothing more to expect from this handler. */
            cmhs->cmhs_Data.memh_Flags &= ~MEMHF_RECYCLE;
        }
    }

    ReleaseSemaphore(&PrivExecBase(SysBase)->LowMemSem);
    return MEM_DID_NOTHING;
}
