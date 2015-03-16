/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>
#include <proto/exec.h>
#include <strings.h>

#include <hardware/bcm283x.h>

/* Linked from kernel.resource,
 * need to retrieve in a cleaner fashion .. */
extern IPTR stack[];

extern void IdleTask(struct ExecBase *);

struct Task *sysIdleTask = NULL;

static int PlatformInit(struct ExecBase *SysBase)
{
    D(bug("[Exec] PlatformInit()\n"));
    
    struct Task *BootTask = SysBase->ThisTask;
    D(bug("[Exec] PlatformInit: Boot Task @ 0x%p\n", BootTask));

    /* for our sanity we will tell exec about the correct stack for the boot task */
    BootTask->tc_SPLower = stack;
    BootTask->tc_SPUpper = stack + STACK_SIZE;

    sysIdleTask = NewCreateTask(TASKTAG_NAME       , "System Idle",
                                TASKTAG_PRI        , -127,
                                TASKTAG_PC         , IdleTask,
                                TASKTAG_ARG1       , SysBase,
                                TAG_DONE);

    sysIdleTask->tc_State      = TS_WAIT;

    D(bug("[Exec] PlatformInit: Idle Task @ 0x%p\n", sysIdleTask));

    return TRUE;
}

ADD2INITLIB(PlatformInit, 0)
