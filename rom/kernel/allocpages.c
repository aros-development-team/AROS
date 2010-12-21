#include <aros/config.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include "memory_intern.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH3(void *, KrnAllocPages,

/*  SYNOPSIS */
	AROS_LHA(void *, addr, A0),
	AROS_LHA(uintptr_t, length, D0),
	AROS_LHA(uint32_t, flags, D1),

/*  LOCATION */
	struct KernelBase *, KernelBase, 27, Kernel)

/*  FUNCTION
	Allocate physical memory pages

    INPUTS
	addr   - Starting address of region which must be included in the
	         allocated region or NULL for the system to choose the
	         starting address. Normally you will supply NULL here.
	length - Length of the memory region to allocate
	flags  - Flags describing type of needed memory. These are the same
		 flags as passed to exec.library/AllocMem().

    RESULT
	Real starting address of the allocated region.

    NOTES
	Since this allocator is page-based, length will always be round up
	to system's memory page size. The same applies to starting address
	(if specified), it will be rounded down to page boundary.

	This function works only on systems with MMU support. Without MMU
	it will always return NULL.

    EXAMPLE

    BUGS

    SEE ALSO
	KrnFreePages()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    APTR res = NULL;
#if USE_MMU
    struct MemHeader *mh;
    ULONG physFlags;
    KRN_MapAttr protection;

    /* We can't work if MMU is not up */
    if (!KernelBase->kb_PageSize)
	return NULL;

    /* Get permissions */
    protection = MAP_Readable|MAP_Writable;
    if (flags & MEMF_EXECUTABLE)
	protection |= MAP_Executable;

    /* Leave only flags that describe physical properties of the memory */
    physFlags = flags & MEMF_PHYSICAL_MASK;

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
	    if (addr >= mh->mh_Lower || addr + length <= mh->mh_Upper + 1)
	    {
		res = krnAllocAbs(mh, addr, length, KernelBase);
		break;
	    }
	}
	else
	{
	    /*
	     * Otherwise try to allocate pages from every MemHeader.
	     * Note that we still may fail if the memory is fragmented too much.
	     */
	    res = krnAllocate(mh, length, flags, KernelBase);
	    if (res)
		break;
	}
    }

    /*
     * The pages we've just allocated have no access rights at all.
     * Now we need to set requested access rights.
     */
    if (res)
    	KrnSetProtection(res, length, protection);
#endif

    return res;

    AROS_LIBFUNC_EXIT
}
