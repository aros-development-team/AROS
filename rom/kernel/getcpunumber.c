#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(unsigned int, KrnGetCPUNumber,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 37, Kernel)

/*  FUNCTION
	Return number of the caller CPU

    INPUTS
	None

    RESULT
	Number of the CPU on which the function is called

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is entirely architecture-specific */
    return 0;

    AROS_LIBFUNC_EXIT
}
