/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/config.h>

#include <kernel_base.h>
#include <kernel_mm.h>

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
    KRN_MapAttr protection;

    /* We can't work if MMU is not up */
    if (!KernelBase->kb_PageSize)
	return NULL;

    /* Get permissions */
    protection = MAP_Readable|MAP_Writable;
    if (flags & MEMF_EXECUTABLE)
	protection |= MAP_Executable;

    res = mm_AllocPages(addr, length, flags, KernelBase);

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
