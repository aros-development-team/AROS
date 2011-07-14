/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME

        Status

    SYNOPSIS

        PROCESS/N,FULL/S,TCB/S,CLI=ALL/S,COM=COMMAND/K

    LOCATION

        C:

    FUNCTION

        Display information about the processes that are executing
	within Shells/CLIs.

    INPUTS

        PROCESS      --  Process Identification number.

        FULL         --  Display all information about the processes.

        TCB          --  As for Full, except that this option omits the
	                 process name.

        CLI=ALL      --  Default. Displays all processes.

        COM=COMMAND  --  Show the process id of the command given. Specify
	                 the command name.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Status

            Process  2: Loaded as command: c:status
            Process  3: Loaded as command: c:NewIcons
            Process  4: Loaded as command: GG:Sys/L/fifo-handler
            Process  5: Loaded as command: Workbench
            Process  6: Loaded as command: ToolsDaemon

        Status full

            Process  2: stk 300000, pri   0 Loaded as command: c:status
            Process  3: stk  4096, pri   0 Loaded as command: c:NewIcons
            Process  4: stk  4096, pri   0 Loaded as command: GG:Sys/L/fifo-handler
            Process  5: stk  6000, pri   1 Loaded as command: Workbench
            Process  6: stk  4000, pri   2 Loaded as command: ToolsDaemon

    BUGS

    SEE ALSO

        <dos/dosextens.h>

    INTERNALS

******************************************************************************/

#include <exec/lists.h>

#define  DEBUG  0
#include <aros/debug.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/types.h>

#include <strings.h>
#include <stdio.h>

#include <aros/shcommands.h>

static void printProcess(struct DosLibrary *DOSBase, BOOL full, BOOL tcb, BOOL all,
			 struct Process *process);

AROS_SH5(Status,41.1,
AROS_SHA(LONG *, ,PROCESS,/N,NULL),
AROS_SHA(BOOL, , FULL,/S,FALSE),
AROS_SHA(BOOL, , TCB,/S,FALSE),
AROS_SHA(BOOL,CLI=,ALL,/S,FALSE),
AROS_SHA(STRPTR,COM=,COMMAND,/K,NULL))
{
    AROS_SHCOMMAND_INIT

    struct RootNode *root = ((struct DosLibrary *)DOSBase)->dl_Root;
    int    retval = RETURN_OK;
    BOOL   full   = SHArg(FULL);
    BOOL   tcb    = SHArg(TCB);
    BOOL   all    = SHArg(ALL);
    ULONG  processNum = 0;
    STRPTR command    = SHArg(COMMAND);


    if (SHArg(PROCESS) != NULL)
    {
        processNum = *SHArg(PROCESS);
    }

    if (!full && !tcb && processNum == 0 && command == NULL)
    {
	all = TRUE;
    }

    if (command != NULL)
    {
 	struct List     *cliList;
	struct CLIInfo  *ci;

	D(bug("command != NULL in Status\n"));

	/* Get access to the rootnode */
	ObtainSemaphore(&root->rn_RootLock);

	D(bug("Got RootLock\n"));

	cliList = (struct List *)&root->rn_CliList;
	ci = (struct CLIInfo *)FindName(cliList, command);

	if (ci != NULL)
	{
	    if (ci->ci_Process->pr_TaskNum != 0)
	    {
		Printf(" %ld\n", ci->ci_Process->pr_TaskNum);
	    }
	}
	else
	{
	    retval = RETURN_WARN;
	}

	ReleaseSemaphore(&root->rn_RootLock);
    }
    else if (processNum != 0)
    {
	struct Process *process;

	ObtainSemaphore(&root->rn_RootLock);

	/* This is a temporary construction until I've fixed the
	   implementation of FindCliProc() */
	Forbid();
	process = FindCliProc(processNum);
	Permit();

	ReleaseSemaphore(&root->rn_RootLock);

	if (process != NULL)
	{
	    printProcess(DOSBase, full, tcb, all, process);
	}
	else
	{
	    Printf("Process %ld does not exist\n", processNum);
	}
    }
    else
    {
	struct List     *cliList;
	struct CLIInfo  *ci;

	ObtainSemaphore(&root->rn_RootLock);

	D(bug("Got RootLock\n"));

	cliList = (struct List *)&root->rn_CliList;

	ForeachNode(cliList, ci)
	{
	    printProcess(DOSBase, full, tcb, all, ci->ci_Process);
	}

	ReleaseSemaphore(&root->rn_RootLock);
    }

    return retval;

    AROS_SHCOMMAND_EXIT
}


/* Print the information for a certain cli process */
static void printProcess(struct DosLibrary *DOSBase, BOOL full, BOOL tcb, BOOL all,
			 struct Process *process)
{
    struct CommandLineInterface *cli = BADDR(process->pr_CLI);

    /* This should never happen, I guess */
    if (cli == NULL)
    {
	return;
    }

    Printf("Process %ld ", process->pr_TaskNum);

    if (tcb || full)
    {
	Printf("stk %lu, pri %ld ",
	       (ULONG)cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT,
	       (LONG)process->pr_Task.tc_Node.ln_Pri);
    }

    if (!tcb || full)
    {
	Printf("Loaded as command: %b", cli->cli_CommandName);
    }

    Printf("\n");
}
