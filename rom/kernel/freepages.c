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
    /* Drop access rights */
    KrnSetProtection(addr, length, 0);
    /* Actually free pages */
    mm_FreePages(addr, length, KernelBase);
#endif

    AROS_LIBFUNC_EXIT
}
