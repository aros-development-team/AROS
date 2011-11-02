/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Page-based memory allocator, low-level routines.
    Lang: english
*/

#include <aros/config.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_mm.h>

#define D(x)

void *mm_AllocPages(void *addr, uintptr_t length, uint32_t flags, struct KernelBase *KernelBase)
{
    APTR res = NULL;
    struct MemHeader *mh;
    ULONG physFlags = flags & MEMF_PHYSICAL_MASK;

    /*
     * Loop over MemHeader structures.
     * We only add MemHeaders and never remove them, so i hope Forbid()/Permit()
     * is not really necessary here.
     */
    ForeachNode(&SysBase->MemList, mh)
    {
	/*
	 * Check for the right requirements and enough free memory.
	 * The requirements are OK if there's no bit in the
	 * 'physFlags' that isn't set in the 'mh->mh_Attributes'.
	 */
	if ((physFlags & ~mh->mh_Attributes) || mh->mh_Free < length)
	   continue;

	if (addr)
	{
	    /*
	     * If we have starting address, only one MemHeader can be
	     * appropriate for us. We look for it and attempt to allocate
	     * the given region from it.
	     */
	    if (addr >= mh->mh_Lower || addr + length <= mh->mh_Upper)
	    {
		res = mm_AllocAbs(mh, addr, length);
		break;
	    }
	}
	else
	{
	    /*
	     * Otherwise try to allocate pages from every MemHeader.
	     * Note that we still may fail if the memory is fragmented too much.
	     */
	    res = mm_Allocate(mh, length, flags);
	    if (res)
		break;
	}
    }

    return res;
}

void mm_FreePages(void *addr, uintptr_t length, struct KernelBase *KernelBase)
{
    struct MemHeader *mh;

    ForeachNode(&SysBase->MemList, mh)
    {
        D(bug("[KrnFreePages] Checking MemHeader 0x%p... ", mh));

	/* Test if the memory belongs to this MemHeader. */
	if (mh->mh_Lower <= addr && mh->mh_Upper > addr)
	{
	    D(bug("[KrnFreePages] Match!\n"));

	    /* Test if it really fits into this MemHeader. */
	    if ((addr + length) > mh->mh_Upper)
		/* Something is completely wrong. */
		Alert(AN_MemCorrupt|AT_DeadEnd);

	    mm_Free(mh, addr, length);
	    break;
	}

	D(bug("[KrnFreePages] No match!\n"));
    }
}

/* Allocate a space usable by exec.library/Allocate() inside kernel's MemHeader. */
struct MemHeader *mm_AllocExecHeader(struct MemHeader *mh, STRPTR name, IPTR maxsize)
{
    struct MemHeader *bootmh = mm_Allocate(mh, maxsize, MEMF_ANY);

    if (bootmh)
    	krnCreateMemHeader(name, mh->mh_Node.ln_Pri, bootmh, maxsize, mh->mh_Attributes);

    return bootmh;
}
