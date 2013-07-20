#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* See rom/kernel/schedule.c for documentation */

AROS_LH0(void, KrnSchedule,
    struct KernelBase *, KernelBase, 6, Kernel)
{
    AROS_LIBFUNC_INIT

    /* The real implementation is in Exec/Schedule */
    Supervisor(__AROS_GETVECADDR(SysBase,7));

    AROS_LIBFUNC_EXIT
}
