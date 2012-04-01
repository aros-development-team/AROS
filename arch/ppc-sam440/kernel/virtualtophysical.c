#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1I(void *, KrnVirtualToPhysical,

/*  SYNOPSIS */
	AROS_LHA(void *, virtual, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 20, Kernel)

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

	uintptr_t virt = (uintptr_t)virtual;
	uintptr_t phys = virt;

	if (virt >= 0xff000000)
		phys = virt - 0xff000000;

	return (void*)phys;

	AROS_LIBFUNC_EXIT
}
