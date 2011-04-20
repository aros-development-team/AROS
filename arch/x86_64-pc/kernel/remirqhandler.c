#define __KERNEL_NOLIBBASE__

#include <proto/kernel.h>

#include "kernel_base.h"

AROS_LH1(void, KrnRemIRQHandler,
         AROS_LHA(void *, handle, A0),
         struct KernelBase *, KernelBase, 8, Kernel)
{
    AROS_LIBFUNC_INIT

    KrnRemExceptionHandler(handle);
    
    AROS_LIBFUNC_EXIT
}
