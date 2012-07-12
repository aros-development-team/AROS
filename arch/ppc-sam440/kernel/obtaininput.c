#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(int, KrnObtainInput,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 33, Kernel)

/*  FUNCTION
	Take over low-level debug input hardware and initialize the input

    INPUTS
	None

    RESULT
	Nonzero for success, zero for failure (for example there's no input channel)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 1;

    AROS_LIBFUNC_EXIT
}
