/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Default trap handler
    Lang: english
*/
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

AROS_UFH1(void, Exec_TrapHandler,
	  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct Task * task;

    task = FindTask (NULL);

    kprintf ( "Traphandler for Task %p (%s) invoked\n"
	, task
	, (task && task->tc_Node.ln_Name) ?
	    task->tc_Node.ln_Name
	    : "-- unknown task --"
    );

    AROS_USERFUNC_EXIT
} /* TrapHandler */
