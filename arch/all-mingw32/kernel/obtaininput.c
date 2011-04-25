#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

AROS_LH0I(int, KrnObtainInput,
	  struct KernelBase *, KernelBase, 33, Kernel)

{
    AROS_LIBFUNC_INIT

    /* Our debug input works out of the box, no preparation needed */
    return 1;

    AROS_LIBFUNC_EXIT
}
