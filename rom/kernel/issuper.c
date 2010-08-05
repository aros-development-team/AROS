#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(int, KrnIsSuper,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 13, Kernel)

/*  FUNCTION
	Determine if the caller is running in supervisor mode

    INPUTS
	None

    RESULT
	Nonzero for supervisor mode, zero for user mode

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is entirely architecture-specific */
    return FALSE;

    AROS_LIBFUNC_EXIT
}
