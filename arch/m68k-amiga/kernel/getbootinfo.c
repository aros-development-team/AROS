#include <aros/kernel.h>

#include <kernel_base.h>

#define DEBUG 0
#include <aros/debug.h>
#include "kernel_debug.h"

#include "exec_intern.h"
#undef KernelBase

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

    D(bug("[Kernel] KrnGetBootInfo()\n"));
    return PrivExecBase(SysBase)->PlatformData.BootMsg;

    AROS_LIBFUNC_EXIT
}
