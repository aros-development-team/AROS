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
