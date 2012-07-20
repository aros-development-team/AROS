#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void, KrnDispatch,

/*  SYNOPSIS */

/*  LOCATION */
         struct KernelBase *, KernelBase, 4, Kernel)

/*  FUNCTION
        Run the next available task

    INPUTS
        None

    RESULT
        None

    NOTES
        This entry point directly calls task dispatch routine in supervisor mode.
        It neither performs any checks of caller status nor obeys interrupt enable
        state. After calling this function, caller's task will be replaced by
        another one, and caller's state will be lost.

        This function is safe to call only from within user mode.
        This function is considered internal, and not meant to be called
        by user's software.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    Supervisor(Dispatch_wapper);

    AROS_LIBFUNC_EXIT
}
