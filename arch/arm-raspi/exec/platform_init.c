/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

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

#include <asm/bcm2835.h>

/* Linked from kernel.resource,
 * need to retrieve in a cleaner fashion .. */
extern IPTR stack[];

static int PlatformInit(struct ExecBase *SysBase)
{
    D(bug("[Exec] PlatformInit()\n"));
    
    struct Task *BootTask = SysBase->ThisTask;
    D(bug("[Exec] PlatformInit: Boot Task @ 0x%p\n", BootTask));

    /* for our sanity we will tell exec about the correct stack for the boot task */
    BootTask->tc_SPLower = stack;
    BootTask->tc_SPUpper = stack + STACK_SIZE;
#if (1)
    // Temp Hack
    BootTask->tc_SPReg = BootTask->tc_SPUpper - sizeof(IPTR);
#endif

    return TRUE;
}

ADD2INITLIB(PlatformInit, 0)
