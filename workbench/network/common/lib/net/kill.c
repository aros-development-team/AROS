#include <dos/dos.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

static BOOL CheckTask(struct List *tl, pid_t pid)
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

    /*
     * The task can be:
     * a) Current one
     * b) In the TaskReady list
     * c) In the TaskWait list
     */
    task_valid = ((pid == (pid_t)FindTask(NULL)) ||
    		  CheckTask(&SysBase->TaskReady, pid) ||
    		  CheckTask(&SysBase->TaskWait, pid));

    if (task_valid && exec_sigs)
	Signal((struct Task *)pid, exec_sigs);

    Permit();

    return task_valid ? 0 : ESRCH;
}

