/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Dos function FindCliProc
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    struct Task *task = NULL;

    /* Okay, simple one first: This task */
    task = FindTask(NULL);
    if (task->tc_Node.ln_Type == NT_PROCESS)
	if (((struct Process *)task)->pr_TaskNum == (LONG)num)
	    return (struct Process *)task;

    /* The ready list */
    ForeachNode(&SysBase->TaskReady, task)
    {
	if (task->tc_Node.ln_Type == NT_PROCESS)
	    if (((struct Process *)task)->pr_TaskNum == (LONG)num)
		return (struct Process *)task;
    }

    /* The waiting list */
    ForeachNode(&SysBase->TaskWait, task)
    {
	if (task->tc_Node.ln_Type == NT_PROCESS)
	    if (((struct Process *)task)->pr_TaskNum == (LONG)num)
		return (struct Process *)task;
    }

    return NULL;
    AROS_LIBFUNC_EXIT
} /* FindCliProc */
