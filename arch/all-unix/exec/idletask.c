/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Idle task.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>

#include <stdlib.h>
#include <signal.h>

void idleTask(struct ExecBase *sysBase)
{
    sigset_t sigs;

    sigemptyset(&sigs);

    while(1)
    {
	sigsuspend(&sigs);
    }
}	    
