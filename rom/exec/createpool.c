/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: Create a memory pool.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <clib/alib_protos.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"
#include "mungwall.h"

/*****************************************************************************

    NAME */
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

        AROS_LH3(APTR, CreatePool,

/*  SYNOPSIS */
        AROS_LHA(ULONG, requirements, D0),
        AROS_LHA(IPTR, puddleSize,   D1),
        AROS_LHA(IPTR, threshSize,   D2),

/*  LOCATION */
        struct ExecBase *, SysBase, 116, Exec)

/*  FUNCTION
        Create a private pool for memory allocations.

    INPUTS
        requirements - The type of the memory
        puddleSize   - The number of bytes that the pool expands by
                   if it is too small.
        threshSize   - Allocations beyond the threshSize are given
                   directly to the system. threshSize must be
                   smaller than or equal to the puddleSize.

    RESULT
        A handle for the memory pool or NULL if the pool couldn't
        be created

    NOTES
        Since exec.library v41.12, the implementation of pools has been
        rewritten to make use of memory protection capabilities. The
        threshSize parameter is effectively ignored and is present only
        for backwards compatibility.

    EXAMPLE
        \* Get the handle to a private memory pool *\
        po=CreatePool(MEMF_ANY,16384,8192);
        if(po!=NULL)
        {
            \* Use the pool *\
            UBYTE *mem1,*mem2;
            mem1=AllocPooled(po,1000);
            mem2=AllocPooled(po,2000);
            \* Do something with the memory... *\

            \* Free everything at once *\
            DeletePool(po);
        }

    BUGS

    SEE ALSO
        DeletePool(), AllocPooled(), FreePooled()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TraceLocation tp = CURRENT_LOCATION("CreatePool");
    struct MemHeader *firstPuddle = NULL;
    IPTR align = PrivExecBase(SysBase)->PageSize - 1;
    ULONG poolstruct_size;
    IPTR headerSize;

    if (align < 4095)
        align = 4095;

    D(bug("[exec] CreatePool(0x%08X, %u, %u)\n", requirements, puddleSize, threshSize);)

    /*
     * puddleSize needs to include MEMHEADER_TOTAL, allocator context size and
     * MEMPOOL_WORSTALIGN.
     * This is because our puddles must be able to accommodate an allocation
     * of own size. Allocations of larger size will always use enlarged puddles.
     * Pointer is used for pointing back to the MemHeader from which the block
     * was allocated, in AllocVec()-alike manner. This way we get rid of slow
     * lookup in FreePooled().
     */
    puddleSize += MEMHEADER_TOTAL + mhac_GetCtxSize() + MEMPOOL_WORSTALIGN;

    /* If mungwall is enabled, count also size of walls, at least for one allocation */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
        puddleSize += MUNGWALL_TOTAL_SIZE;

    /* Then round puddleSize up to be a multiple of page size. */
    puddleSize = (puddleSize + align) & ~align;
    D(bug("[CreatePool] Aligned puddle size: %u (0x%08X)\n", puddleSize, puddleSize);)

    poolstruct_size = (requirements & MEMF_SEM_PROTECTED) ? sizeof(struct ProtectedPool) :
                                      sizeof(struct Pool);

    /*
     * The pool handle is the address of a MemHeader whose first allocation
     * is the pool structure (see AllocPooled()). Historically that
     * MemHeader was a full puddle, which made every pool cost puddleSize
     * bytes up front even when nothing was ever allocated from it - and
     * pools are created eagerly all over the system. Instead, size the
     * handle's block for the pool structure (plus its allocator context)
     * alone and leave the rest of the puddle list empty: the first
     * AllocPooled() brings in the first real puddle. The block is laid
     * out exactly like a puddle and sits on the puddle list as before,
     * so nothing downstream changes; with zero free bytes it can never
     * satisfy an allocation nor be reclaimed as an empty puddle.
     */
    headerSize = MEMHEADER_TOTAL + mhac_GetCtxSize()
        + ((poolstruct_size + MEMCHUNK_TOTAL - 1) & ~(MEMCHUNK_TOTAL - 1));

    firstPuddle = nommu_AllocMem(headerSize, requirements & ~MEMF_SEM_PROTECTED, &tp, SysBase);
    if (firstPuddle)
    {
        struct MemHeader *orig = FindMem(firstPuddle, SysBase);

        if (IsManagedMem(orig))
        {
            /*
             * In managed memory the block itself becomes the managed pool
             * and needs real capacity behind the header, so managed pools
             * keep the historical eager full-size block.
             */
            nommu_FreeMem(firstPuddle, headerSize, &tp, SysBase);
            firstPuddle = AllocMemHeader(puddleSize, requirements & ~MEMF_SEM_PROTECTED, &tp, SysBase);
        }
        else
        {
            /* Initialize the handle's MemHeader like AllocMemHeader() would */
            firstPuddle->mh_Node.ln_Type = NT_MEMORY;
            firstPuddle->mh_Node.ln_Pri  = orig->mh_Node.ln_Pri;
            firstPuddle->mh_Attributes   = orig->mh_Attributes;
            firstPuddle->mh_Lower        = (APTR)firstPuddle + MEMHEADER_TOTAL;
            firstPuddle->mh_Upper        = (APTR)firstPuddle + headerSize;
            firstPuddle->mh_First        = firstPuddle->mh_Lower;
            firstPuddle->mh_Free         = headerSize - MEMHEADER_TOTAL;

            firstPuddle->mh_First->mc_Next  = NULL;
            firstPuddle->mh_First->mc_Bytes = firstPuddle->mh_Free;
        }
    }
    D(bug("[CreatePool] Pool header block 0x%p\n", firstPuddle);)

    if (firstPuddle)
    {
        struct ProtectedPool *pool;

        /*
         * Allocate pool header inside the puddle.
         * It is the first allocation in this puddle, so in future we can always find
         * header's address as poolbase + MEMHEADER_TOTAL.
         */
        pool = Allocate(firstPuddle, poolstruct_size);
        D(bug("[CreatePool] Pool header 0x%p (size %u)\n", pool, poolstruct_size);)

        /* Initialize pool header */
        NEWLIST((struct List *)&pool->pool.PuddleList);
        pool->pool.Requirements = requirements;
        pool->pool.PuddleSize   = puddleSize;
        pool->pool.PoolMagic   = POOL_MAGIC;

        if (requirements & MEMF_SEM_PROTECTED)
        {
            InitSemaphore(&pool->sem);
        }

        /*
         * If the pool is in managed memory, don't bother any further setup. The
         * pool should do the rest self.
         */
        if (IsManagedMem(firstPuddle))
        {
            D(bug("Managed pool\n");)
            /*
             * Just link the pool structure at the ln_Name - we will need that
             * for the semaphore
             */
            if (requirements & MEMF_SEM_PROTECTED)
            {
                firstPuddle->mh_Node.ln_Name = (STRPTR)&pool->sem;
                firstPuddle->mh_First = (APTR)((IPTR)firstPuddle->mh_First | MEMF_SEM_PROTECTED);
            }
        }
        else
        {
            /*
             * Add the puddle to the list (yes, contained in itself).
             * This is the first puddle so it's safe to use AddTail() here.
             * Note that we use ln_Name of our MemHeader to point back to
             * our pool (directly or indirectly).
             */
            mhac_PoolMemHeaderSetup(firstPuddle, pool);
            AddTail((struct List *)&pool->pool.PuddleList, &firstPuddle->mh_Node);
        }
    }
    return firstPuddle;

    AROS_LIBFUNC_EXIT
} /* CreatePool */
