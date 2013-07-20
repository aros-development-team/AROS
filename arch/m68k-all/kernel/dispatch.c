#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* See rom/kernel/dispatch.c for documentation */

AROS_LH0(void, KrnDispatch,
    struct KernelBase *, KernelBase, 4, Kernel)
{
    AROS_LIBFUNC_INIT

    /* The real implementation is in Exec/Dispatch */
    Supervisor(__AROS_GETVECADDR(SysBase,10));

    AROS_LIBFUNC_EXIT
}
