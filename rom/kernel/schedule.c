#include "kernel_base.h"
#include "kernel_syscall.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void, KrnSchedule,

/*  SYNOPSIS */

/*  LOCATION */
         struct KernelBase *, KernelBase, 6, Kernel)

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

    krnSysCall(SC_SCHEDULE);

    AROS_LIBFUNC_EXIT
}
