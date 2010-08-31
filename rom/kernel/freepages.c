#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(void, KrnFreePages,

/*  SYNOPSIS */
	AROS_LHA(void *, phy_addr, A0),
	AROS_LHA(uint32_t, length, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 28, Kernel)

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

    /* Just a reserved function for now */

    AROS_LIBFUNC_EXIT
}
