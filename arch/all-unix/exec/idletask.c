/*
    Copyright (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Idle task.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>

#include <stdlib.h>
#include <sys/signal.h>

void idleTask(struct ExecBase *sysBase)
{
    sigset_t sigs;

    sigemptyset(&sigs);

    while(1)
    {
	sigsuspend(&sigs);
    }
}	    
