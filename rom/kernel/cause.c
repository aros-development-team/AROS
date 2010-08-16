#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(void, KrnCause,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 3, Kernel)

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

    krnSysCall(SC_CAUSE);

    AROS_LIBFUNC_EXIT
}
