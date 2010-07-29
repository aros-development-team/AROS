#include <aros/libcall.h>

#include "kernel_base.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(int, KrnIsSuper,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 13, Kernel)

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

    return FALSE;

    AROS_LIBFUNC_EXIT
}
