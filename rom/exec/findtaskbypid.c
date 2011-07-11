#include <exec/execbase.h>
#include <exec/tasks.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(struct Task *, FindTaskByPID,

/*  SYNOPSIS */
	AROS_LHA(ULONG, id, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 166, Exec)

/*  FUNCTION
	Scan through the task lists searching for the task whose
	et_UniqueID field matches.

    INPUTS
	id	-   The task ID to match.

    RESULT
	Address of the Task control structure that matches, or
	NULL otherwise.

    NOTES
    	This function is source-compatible with MorphOS.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *t;
    struct ETask *et;

    /*
	First up, check ThisTask. It could be NULL because of exec_init.c
    */
    if (SysBase->ThisTask != NULL)
    {
	et = GetETask(SysBase->ThisTask);
	if (et != NULL && et->et_UniqueID == id)
	    return SysBase->ThisTask;
    }

    /*	Next, go through the ready list */
    ForeachNode(&SysBase->TaskReady, t)
    {
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
	    return t;
    }

    /* Finally, go through the wait list */
    ForeachNode(&SysBase->TaskWait, t)
    {
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
	    return t;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}
