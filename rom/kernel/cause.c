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
        Run software interrupt processing sequence

    INPUTS
        None

    RESULT
        None

    NOTES
        This entry point directly calls interrupt processing routine
        in supervisor mode. It neither performs any checks of caller status
        nor obeys interrupt enable state.

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

    krnSysCall(SC_CAUSE);

    AROS_LIBFUNC_EXIT
}
