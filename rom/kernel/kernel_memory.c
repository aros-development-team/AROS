/*
    Copyright � 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common memory utility functions
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"

#define USE_TLSF
#ifdef USE_TLSF

#include "tlsf.h"

static void destroy_Pool(struct MemHeaderExt *mhe)
{
    tlsf_destroy(mhe->mhe_UserData);
}

static APTR fetch_more_ram(void * data, IPTR *size)
{
    struct MemHeaderExt *mhe = (struct MemHeaderExt *)data;

    D(nbug("[TLSF] fetch_more_ram(%p, %d)\n", mhe, *size));

    APTR ptr = AllocMem(*size, mhe->mhe_MemHeader.mh_Attributes);
    return ptr;
}

static VOID release_ram(void * data, APTR ptr, IPTR size)
{
    D(nbug("[TLSF] release_ram(%p, %d)\n", ptr, size));

    FreeMem(ptr, size);
}

static void * init_Pool(struct MemHeaderExt *mhe, IPTR puddleSize, IPTR initialSize)
{
    return tlsf_init_autogrow(mhe->mhe_UserData, initialSize, puddleSize,
            fetch_more_ram, release_ram, mhe);
}

static APTR alloc_mem(struct MemHeaderExt *mhe, IPTR  size,  ULONG *flags)
{
    void *ptr;

    D({
        struct ExecBase *b = SysBase;

        nbug("[TLSF] alloc_mem(%p, %d, %p(%d)), tlsf=%p, Task %p (%s)\n",
            mhe, size, flags, flags? *flags : -1, mhe->mhe_UserData,
            b ? b->ThisTask : NULL,
            b ? (b->ThisTask ? b->ThisTask->tc_Node.ln_Name : "unknown") : "no_exec")
    });

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ObtainSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    ptr = tlsf_malloc(mhe->mhe_UserData, size);

    mhe->mhe_MemHeader.mh_Free = tlsf_avail(mhe->mhe_UserData, MEMF_TOTAL);

    D(nbug("[TLSF] alloc returned %p\n", ptr));

    if (flags && (*flags & MEMF_CLEAR))
    {
        bzero(ptr, size);
    }

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    return ptr;
}

static void free_mem(struct MemHeaderExt *mhe, APTR ptr, IPTR size)
{
    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ObtainSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    D(nbug("[TLSF] free mem (%p, %p, %d), tlsf=%p\n",
            mhe, ptr, size, mhe->mhe_UserData));

    if (ptr && size)
        tlsf_free(mhe->mhe_UserData, ptr);

    mhe->mhe_MemHeader.mh_Free = tlsf_avail(mhe->mhe_UserData, MEMF_TOTAL);

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);
}

static void free_vec(struct MemHeaderExt *mhe, APTR ptr)
{
    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ObtainSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    D(nbug("[TLSF] free vec (%p, %p), tlsf=%p\n",
            mhe, ptr, mhe->mhe_UserData));

    if (ptr)
        tlsf_free(mhe->mhe_UserData, ptr);

    mhe->mhe_MemHeader.mh_Free = tlsf_avail(mhe->mhe_UserData, MEMF_TOTAL);

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);
}

static APTR alloc_abs(struct MemHeaderExt *mhe, IPTR size, APTR location)
{
    void *ptr = NULL;

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ObtainSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    D(nbug("[TLSF] alloc abs (%p, %p, %d), tlsf=%p\n",
            mhe, location, size, mhe->mhe_UserData));

    if (location && size)
        ptr = tlsf_allocabs(mhe->mhe_UserData, location, size);

    mhe->mhe_MemHeader.mh_Free = tlsf_avail(mhe->mhe_UserData, MEMF_TOTAL);

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    return ptr;
}

static APTR _realloc(struct MemHeaderExt *mhe, APTR location, IPTR size)
{
    void *ptr;

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ObtainSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    D(nbug("[TLSF] realloc (%p, %p, %d), tlsf=%p\n",
            mhe, location, size, mhe->mhe_UserData));

    ptr =  tlsf_realloc(mhe->mhe_UserData, location, size);

    mhe->mhe_MemHeader.mh_Free = tlsf_avail(mhe->mhe_UserData, MEMF_TOTAL);

    D(nbug("[TLSF]   realloc returned %p\n", ptr));

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    return ptr;
}

static IPTR avail(struct MemHeaderExt *mhe, ULONG requirements)
{
    return tlsf_avail(mhe->mhe_UserData, requirements);
}

static BOOL inbounds(struct MemHeaderExt *mhe, APTR begin, APTR end)
{
    return tlsf_in_bounds(mhe->mhe_UserData, begin, end);
}

/*
 * Create MemHeader structure for the specified RAM region.
 * The header will be placed in the beginning of the region itself.
 * The header will NOT be added to the memory list!
 */
void krnCreateMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags)
{
    /* The MemHeader itself does not have to be aligned */
    struct MemHeader *mh = start;
    struct MemHeaderExt *mhe = start;

    /* If the end is less than (1 << 31), MEMF_31BIT is implied */
    if (((IPTR)start+size) < (1UL << 31))
        flags |= MEMF_31BIT;
    else
        flags &= ~MEMF_31BIT;

    mhe->mhe_DestroyPool   = destroy_Pool;
    mhe->mhe_InitPool      = init_Pool;

    mhe->mhe_Alloc         = alloc_mem;
    mhe->mhe_AllocVec      = alloc_mem;
    mhe->mhe_Free          = free_mem;
    mhe->mhe_FreeVec       = free_vec;
    mhe->mhe_AllocAbs      = alloc_abs;
    mhe->mhe_ReAlloc       = _realloc;
    mhe->mhe_Avail         = avail;
    mhe->mhe_InBounds      = inbounds;

    mh->mh_Node.ln_Succ    = NULL;
    mh->mh_Node.ln_Pred    = NULL;
    mh->mh_Node.ln_Type    = NT_MEMORY;
    mh->mh_Node.ln_Name    = (STRPTR)name;
    mh->mh_Node.ln_Pri     = pri;
    mh->mh_Attributes      = flags | MEMF_MANAGED;
    /* The first MemChunk needs to be aligned. We do it by adding MEMHEADER_TOTAL. */
    mh->mh_First           = NULL;
    mh->mh_Lower           = start;
    mh->mh_Upper           = start + size;

    mhe->mhe_UserData      = (APTR)mh + ((sizeof(struct MemHeaderExt) + 15) & ~15);

    tlsf_init(mhe->mhe_UserData, size - ((sizeof(struct MemHeaderExt) + 15) & ~15));

    /*
     * mh_Lower and mh_Upper are informational only. Since our MemHeader resides
     * inside the region it describes, the region includes MemHeader.
     */
    mh->mh_Lower           = start;
    mh->mh_Upper           = start + size;
    mh->mh_Free            = size;
}


#else

/*
 * Create MemHeader structure for the specified RAM region.
 * The header will be placed in the beginning of the region itself.
 * The header will NOT be added to the memory list!
 */
void krnCreateMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags)
{
    /* The MemHeader itself does not have to be aligned */
    struct MemHeader *mh = start;

    /* If the end is less than (1 << 31), MEMF_31BIT is implied */
    if (((IPTR)start+size) < (1UL << 31))
        flags |= MEMF_31BIT;
    else
        flags &= ~MEMF_31BIT;

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
    mh->mh_Upper           = start + size;
    mh->mh_Free            = mh->mh_First->mc_Bytes;
}
#endif

/*
 * Create informational MemHeader for ROM region.
 * The header will be allocated from system's public memory lists.
 * It will be not possible to allocate memory from the created MemHeader.
 * The header will be added to the memory list.
 * This routine uses exec.library/Allocate() for memory allocation, so it is safe
 * to use before exec.library and kernel.resource memory management is initialized.
 */
struct MemHeader *krnCreateROMHeader(CONST_STRPTR name, APTR start, APTR end)
{
    struct MemHeader *mh = AllocMem(sizeof(struct MemHeader), MEMF_ANY);

    if (mh)
    {
	mh->mh_Node.ln_Type = NT_MEMORY;
	mh->mh_Node.ln_Name = (STRPTR)name;
	mh->mh_Node.ln_Pri = -128;
	mh->mh_Attributes = MEMF_KICK;
	mh->mh_First = NULL;
	mh->mh_Lower = start;
	mh->mh_Upper = end + 1;			/* end is the last valid address of the region */
	mh->mh_Free = 0;                        /* Never allocate from this chunk! */
	Enqueue(&SysBase->MemList, &mh->mh_Node);
    }

    return mh;
}
