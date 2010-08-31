#include <exec/execbase.h>
#include <proto/exec.h>

#include <signal.h>
#include <unistd.h>

#include <kernel_base.h>

AROS_LH0(void, KrnDispatch,
         struct KernelBase *, KernelBase, 4, Kernel)
{
    AROS_LIBFUNC_INIT

    sigset_t temp_sig_int_mask;

    sigemptyset(&temp_sig_int_mask);	
    sigaddset( &temp_sig_int_mask, SIGUSR1);

    /* 
     * It's quite possible that they have interrupts Disabled(),
     * we should fix that here, otherwise we can't switch. 
     *
     * We can't call the dispatcher because we need a signal,
     * lets just create one.
     *
     * Have to set the dispatch-required flag.
     * I use SIGUSR1 (maps to SoftInt) because it has less effect on
     * the system clock, and is probably quicker.
     */
    sigprocmask(SIG_UNBLOCK, &temp_sig_int_mask, NULL);
    SysBase->AttnResched |= ARF_AttnDispatch;
    kill(getpid(), SIGUSR1);
    sigprocmask(SIG_BLOCK, &temp_sig_int_mask, NULL);

    AROS_LIBFUNC_EXIT
}
