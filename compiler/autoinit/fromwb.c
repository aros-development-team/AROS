/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - handle program started from workbench.
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <aros/startup.h>
#include <aros/symbolsets.h>

#define DEBUG 0
#include <aros/debug.h>

struct WBStartup *WBenchMsg;

int __nowbsupport __attribute__((weak)) = 0;

static void __startup_fromwb(void)
{
    struct Process *myproc;

    D(bug("Entering __startup_fromwb()\n"));

    myproc = (struct Process *)FindTask(NULL);

    /* Do we have a CLI structure? */
    if (!myproc->pr_CLI)
    {
	/* Workbench startup. Get WBenchMsg and pass it to main() */

	WaitPort(&myproc->pr_MsgPort);
	WBenchMsg = (struct WBStartup *)GetMsg(&myproc->pr_MsgPort);
	__argv = (char **) WBenchMsg;
        __argc = 0;

	D(bug("[startup] Started from Workbench\n"));
    }

    __startup_entries_next();

    /* Reply startup message to Workbench */
    if (WBenchMsg)
    {
        Forbid(); /* make sure we're not UnLoadseg()ed before we're really done */
        ReplyMsg((struct Message *) WBenchMsg);
    }

    D(bug("Leaving __startup_fromwb\n"));
}

ADD2SET(__startup_fromwb, program_entries, -50);
