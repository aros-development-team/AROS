#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1(void *, KrnAllocPages,

/*  SYNOPSIS */
	AROS_LHA(uint32_t, length, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 27, Kernel)

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

    /* Just a reserved function for now */
    return NULL;

    AROS_LIBFUNC_EXIT
}
