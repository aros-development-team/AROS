#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(void, KrnSti,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 10, Kernel)

/*  FUNCTION
	Instantly enable interrupts.

    INPUTS
	None

    RESULT
	None

    NOTES
	This is low level function, it does not have nesting count
	and state tracking mechanism. It operates directly on the CPU.
	Normal applications should consider using exec.library/Enable().

    EXAMPLE

    BUGS

    SEE ALSO
	KrnCli()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    krnSysCall(SC_STI);

    AROS_LIBFUNC_EXIT
}
