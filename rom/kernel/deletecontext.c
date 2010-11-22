#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_memory.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1(void, KrnDeleteContext,

/*  SYNOPSIS */
	AROS_LHA(void *, context, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 19, Kernel)

/*  FUNCTION
	Free CPU context storage area

    INPUTS
    	context - a pointer to a CPU context storage previously allocated using
    		  KrnCreateContext()

    RESULT
    	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    	KrnCreateContext()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    cpumode_t mode = goSuper();

    krnFreeMem(context, KernelBase->kb_ContextSize);

    goBack(mode);

    AROS_LIBFUNC_EXIT
}
