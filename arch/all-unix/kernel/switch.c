#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <proto/kernel.h>

AROS_LH0(void, KrnSwitch,
         struct KernelBase *, KernelBase, 5, Kernel)
{
    AROS_LIBFUNC_INIT

    Disable();

    /* If the state is not TS_RUN then the task is already in a list */
    if (SysBase->ThisTask->tc_State != TS_RUN)
        /*
         * Cause actual task switch. Since we do not have enough
         * signals to map all syscalls, switch and dispatch are
         * both processed by the same SIGUSR1. Actual mode is
         * determined by tc_State. If it's TS_REMOVED, this will
         * be KrnDispatch(), otherwise this is KrnSwitch()
         */
	KrnDispatch();

    Enable();

    AROS_LIBFUNC_EXIT
}
