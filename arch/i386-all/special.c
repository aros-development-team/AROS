/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Default trap handler
    Lang: english
*/
#include <exec/tasks.h>
#include <proto/exec.h>
#include <stdio.h>

AROS_LH0(void, TrapHandler,
    struct ExecBase *, SysBase, NONE, Exec)
{
    struct Task * task;

    task = FindTask (NULL);

    fprintf (stderr
	, "Traphandler for Task %p (%s) invoked\n"
	, task
	, (task && task->tc_Node.ln_Name) ?
	    task->tc_Node.ln_Name
	    : "-- unknown task --"
    );
    fflush (stderr);
} /* TrapHandler */
