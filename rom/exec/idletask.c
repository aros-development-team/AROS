/*
    Copyright (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Idle task.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>

void idleTask(struct ExecBase *SysBase)
{
    while(1)
    {
	/*
	    If you can do something here that doesn't busy wait,
	    you should reimplement it elsewhere.

	    See config/unix/exec/idletask.c for what I mean.
	*/
    }
}
