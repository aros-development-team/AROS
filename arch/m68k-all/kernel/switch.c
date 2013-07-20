#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* See rom/kernel/switch.c for documentation */

AROS_LH0(void, KrnSwitch,
    struct KernelBase *, KernelBase, 5, Kernel)
{
    AROS_LIBFUNC_INIT

    /* The real implementation is in Exec/Switch */
    Supervisor(__AROS_GETVECADDR(SysBase,9));

    AROS_LIBFUNC_EXIT
}
