#include <dos/dos.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <errno.h>

BOOL CheckTask(struct List *tl, pid_t pid)
{
    struct Node *t;
    for (t = tl->lh_Head; t->ln_Succ; t = t->ln_Succ)
	if ((pid_t)t == pid)
	    return TRUE;
    return FALSE;
}

int kill(pid_t pid, int sigs)
{
    ULONG exec_sigs;
    int task_valid;

    if ((pid == 0) || (sigs < 0))
	return ESRCH;
    switch (sigs) {
    case SIGTERM:
	exec_sigs = SIGBREAKF_CTRL_C;
    case 0:
	exec_sigs = 0;
	break;
    default:
	return EINVAL;
    }
    Forbid();
    task_valid = (CheckTask(&SysBase->TaskReady, pid) || CheckTask(&SysBase->TaskWait, pid));
    if (task_valid && exec_sigs)
	Signal((struct Task *)pid, exec_sigs);
    Permit();
    return task_valid ? 0 : ESRCH;
}

