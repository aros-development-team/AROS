/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common memory utility functions
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <kernel_base.h>

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
