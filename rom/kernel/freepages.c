#include <aros/config.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include "memory_intern.h"

#define D(x)

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(void, KrnFreePages,

/*  SYNOPSIS */
	AROS_LHA(void *, addr, A0),
	AROS_LHA(uintptr_t, length, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 28, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
    	This function works only on systems with MMU support.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if USE_MMU
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

	    krnFree(mh, addr, length, KernelBase);
	    break;
	}

	D(bug("[KrnFreePages] No match!\n"));
    }
#endif

    AROS_LIBFUNC_EXIT
}
