#include <aros/libcall.h>

#include "kernel_base.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1I(void *, KrnVirtualToPhysical,

/*  SYNOPSIS */
	AROS_LHA(void *, virtual, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 20, Kernel)
{
	AROS_LIBFUNC_INIT

	return virtual;

	AROS_LIBFUNC_EXIT
}
