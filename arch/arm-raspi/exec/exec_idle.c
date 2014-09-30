/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include "exec_intern.h"

void IdleTask(struct ExecBase *SysBase)
{
    D(bug("[Kernel] Idle task started up"));

    do
    { /* forever */
        D(bug("[IDLE] Nothing to do ..\n"));
    } while(1);
}
