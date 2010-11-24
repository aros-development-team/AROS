#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include "memory_intern.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH3(void *, KrnAllocPages,

/*  SYNOPSIS */
	AROS_LHA(uint32_t, length, D0),
	AROS_LHA(uint32_t, flags, D1),
	AROS_LHA(KRN_MapAttr, protection, D2),

/*  LOCATION */
	struct KernelBase *, KernelBase, 27, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemHeader *mh;
    APTR res = NULL;

    /* Leave only flags that describe physical properties of the memory */
    flags &= (MEMF_PUBLIC|MEMF_CHIP|MEMF_FAST|MEMF_LOCAL|MEMF_24BITDMA);

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
	 * 'flags' that isn't set in the 'mh->mh_Attributes'.
	 */
	if ((flags & ~mh->mh_Attributes) || mh->mh_Free < length)
	   continue;

	/*
	 * Try to allocate pages from the MemHeader.
	 * Note that we still may fail if the memory is fragmented too much.
	 */
        res = krnAllocate(mh, length, KernelBase);
	if (res)
	{
	    /*
	     * The pages we've just allocated have no access rights at all.
	     * Now we need to set requested access rights.
	     */
	    KrnSetProtection(res, length, protection);
	    break;
	}
    }

    return res;

    AROS_LIBFUNC_EXIT
}
