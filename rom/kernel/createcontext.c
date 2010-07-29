#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_cpu.h"
#include "kernel_memory.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(void *, KrnCreateContext,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 18, Kernel)

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

    void *ctx;
    cpumode_t mode = goSuper();

    ctx = AllocKernelMem(sizeof(struct AROSCPUContext));

    goBack(mode);

    return ctx;

    AROS_LIBFUNC_EXIT
}
