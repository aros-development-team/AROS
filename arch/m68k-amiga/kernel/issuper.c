#include <aros/kernel.h>
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
	Callers should only test the return value against zero.
	Nonzero values may actually be different, since they
	may carry some private implementation-dependent information
	(like CPU privilege level, for example).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    register ULONG sr;

    asm volatile ("move.w %%sr,%0" : "=g" (sr) : :);

    return (sr & (1 << 13)) ? TRUE : FALSE;

    AROS_LIBFUNC_EXIT
}
