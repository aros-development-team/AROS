/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Find a CLI process by number
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(struct Process *, FindCliProc,

/*  SYNOPSIS */
	AROS_LHA(ULONG, num, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 91, Dos)

/*  FUNCTION
	Find a CLI process by its task number. The number must be greater
	than 0. 

    INPUTS
	num - The task number of the CLI to find.

    RESULT
	Pointer to the process if found, NULL otherwise.

    NOTES
	Please don't use this function. If you use it, be sure to call
	Forbid() first.

    EXAMPLE

    BUGS

    SEE ALSO
	Cli(), Forbid(), MaxCli()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    struct Process *proc;

    /* Okay, simple one first: This task */
    proc = (struct Process *)FindTask(NULL);
    if (proc->pr_Task.tc_Node.ln_Type == NT_PROCESS &&
	proc->pr_TaskNum &&
	proc->pr_TaskNum == (LONG)num)
    {
	return proc;
    }

    /* The ready list */
    ForeachNode(&SysBase->TaskReady, proc)
    {
	if (proc->pr_Task.tc_Node.ln_Type == NT_PROCESS &&
	    proc->pr_TaskNum &&
	    proc->pr_TaskNum == (LONG)num)
	{
	    return proc;
	}
    }

    /* The waiting list */
    ForeachNode(&SysBase->TaskWait, proc)
    {
	if (proc->pr_Task.tc_Node.ln_Type == NT_PROCESS &&
	    proc->pr_TaskNum &&
	    proc->pr_TaskNum == (LONG)num)
	{
	    return proc;
	}
    }

    return NULL;
    AROS_LIBFUNC_EXIT
} /* FindCliProc */
