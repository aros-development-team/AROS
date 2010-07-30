#include <kernel_base.h>

/*
 * We store boot message in a global variable because we need to store it before
 * we get KernelBase
 */
struct TagItem *BootMsg = NULL;

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(struct TagItem *, KrnGetBootInfo,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 11, Kernel)

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

    return BootMsg;

    AROS_LIBFUNC_EXIT
}
