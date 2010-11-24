#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/memory.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1(void, KrnInitMemory,

/*  SYNOPSIS */
	AROS_LHA(struct MemHeader *, mh, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 31, Kernel)

/*  FUNCTION
	Initialize kernel memory management on a given memory region

    INPUTS
    	mh - Address of a filled in structure describing the region.

    RESULT
    	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR align = KernelBase->kb_PageSize - 1;
    APTR addr;
    IPTR len;

    /*
     * Align start and end addresses on page boundaries.
     * We use mh_First as start address of our usable area
     * and mh_Free to count usable free space.
     * We don't touch physical boundaries (mh_Lower and mh_Upper)
     * and also take into account the fact that some space from this
     * MemHeader could have been used by boot-time allocator and
     * the MemHeader structure itself.
     */
    addr = (APTR)(((IPTR)mh->mh_First + align) & ~align);
    len = mh->mh_Upper - addr + 1;
    len &= ~align;

    /* Set new start and length of the free space */
    mh->mh_First = addr;
    mh->mh_Free = len;

    /* Set up the initial MemChunk */
    mh->mh_First->mc_Next = NULL;
    mh->mh_First->mc_Bytes = len;

    /* The first page from the region will be read-only (we want to be able to read the MemChunk) */
    KrnSetProtection(addr, KernelBase->kb_PageSize, MAP_Readable);
    /* The rest will be unaccessible at all */
    KrnSetProtection(addr + KernelBase->kb_PageSize, len - KernelBase->kb_PageSize, 0);

    AROS_LIBFUNC_EXIT
}
