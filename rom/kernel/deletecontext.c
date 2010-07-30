#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_memory.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1I(void, KrnDeleteContext,

/*  SYNOPSIS */
	AROS_LHA(void *, context, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 19, Kernel)

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

    cpumode_t mode = goSuper();

    krnFreeMem(context, sizeof(struct AROSCPUContext));

    goBack(mode);

    AROS_LIBFUNC_EXIT
}
