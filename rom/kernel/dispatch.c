#include "kernel_base.h"
#include "kernel_syscall.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void, KrnDispatch,

/*  SYNOPSIS */

/*  LOCATION */
         struct KernelBase *, KernelBase, 4, Kernel)

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

    KernelSysCall(SC_DISPATCH);

    AROS_LIBFUNC_EXIT
}
