/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$

    Desc: Default trap handler
    Lang: english
*/
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

AROS_LH0(void, TrapHandler,
    struct ExecBase *, SysBase, NONE, Exec)
{
    struct Task * task;

    task = FindTask (NULL);

    kprintf ( "Traphandler for Task %p (%s) invoked\n"
	, task
	, (task && task->tc_Node.ln_Name) ?
	    task->tc_Node.ln_Name
	    : "-- unknown task --"
    );
} /* TrapHandler */
