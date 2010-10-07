/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Idle task.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <aros/kernel.h>
#include <proto/arossupport.h>

void idleTask(struct ExecBase *sysBase)
{
  KRNWireImpl(IdleTask);
  CALLHOOKPKT(krnIdleTaskImpl,0,0);
}	    
