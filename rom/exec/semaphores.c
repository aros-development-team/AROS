#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "exec_util.h"
#include "semaphores.h"

BOOL CheckSemaphore(struct SignalSemaphore *sigSem, struct TraceLocation *caller)
{
    /* TODO: Introduce AlertContext for this */

    if (KernelBase && KrnIsSuper())
    {
	/* FindTask() is called only here, for speedup */
        struct Task *me = FindTask(NULL);

        kprintf("%s called in supervisor mode!!!\n"
	        "sem = %x  task = %x (%s)\n\n", sigSem, me, me->tc_Node.ln_Name);
	Exec_ExtAlert(ACPU_PrivErr & ~AT_DeadEnd, __builtin_return_address(0), CALLER_FRAME, 0, NULL, SysBase);

	return FALSE;
    }

    if (sigSem->ss_Link.ln_Type != NT_SIGNALSEM)
    {
        struct Task *me = FindTask(NULL);

        /* TODO: Turn this into alert context */
        kprintf("\n\nObtainSemaphore called on a not intialized semaphore!!! "
	        "sem = %x  task = %x (%s)\n\n", sigSem, me, me->tc_Node.ln_Name);
	Exec_ExtAlert(AN_SemCorrupt, __builtin_return_address(0), CALLER_FRAME, 0, NULL, SysBase);

	return FALSE; /* A crude attempt to recover... */
    }

    return TRUE;
}
