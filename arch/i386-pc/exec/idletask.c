/*
    Copyright (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Idle task.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>

void idleTask(struct ExecBase *sysBase)
{
    while(1)
    {
	asm ("hlt");
    }
}	    
