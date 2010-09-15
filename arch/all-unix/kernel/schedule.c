#include <aros/atomic.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <kernel_base.h>

AROS_LH0(void, KrnSchedule,
         struct KernelBase *, KernelBase, 6, Kernel)
{
    AROS_LIBFUNC_INIT

    AROS_ATOMIC_AND(SysBase->AttnResched, ~ARF_AttnSwitch);

    /* Move the current task away. */
    SysBase->ThisTask->tc_State=TS_READY;
    Enqueue(&SysBase->TaskReady,&SysBase->ThisTask->tc_Node);

    /* And force a rescedule. */
    KrnSwitch();

    AROS_LIBFUNC_EXIT
}
