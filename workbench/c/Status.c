/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Status CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        Status

    SYNOPSIS

        PROCESS/N,FULL/S,TCB/S,CLI=ALL/S,COM=COMMAND/K

    LOCATION

        Workbench:c

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

    HISTORY

    17.11.2000  --  SDuvan, rewrote from scratch as the old version was
                    very buggy and overly complex

******************************************************************************/

#define  AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>

#define  DEBUG  0
#include <aros/debug.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <exec/types.h>

#include <strings.h>
#include <stdio.h>

#define ARG_TEMPLATE    "PROCESS/N,FULL/S,TCB/S,CLI=ALL/S,COM=COMMAND/K"

enum 
{
    ARG_PROCESS =  0,
    ARG_FULL,
    ARG_TCB,
    ARG_ALL,
    ARG_COMMAND,
    NOOFARGS
};


void printProcess(BOOL full, BOOL tcb, struct Process *process);


static const char version[] = "$VER: Status 41.1 (17.11.2000)\n";

int __nocommandline = 1;

int main(void)
{
    struct RootNode *root = ((struct DosLibrary *)DOSBase)->dl_Root;

    struct RDArgs *rda;
    IPTR           args[NOOFARGS] = { (IPTR)0, (IPTR)FALSE, (IPTR)FALSE,
				      (IPTR)FALSE, NULL };
    
    int  retval = RETURN_OK;

    D(bug("Status: Doing ReadArgs()\n"));
    
    rda = ReadArgs(ARG_TEMPLATE, args, NULL);

    if(rda != NULL)
    {
	BOOL   full    = (BOOL)args[ARG_FULL];
	BOOL   tcb     = (BOOL)args[ARG_TCB];
	BOOL   all     = (BOOL)args[ARG_ALL];    /* It seems like this is not
						    needed */
	ULONG  processNum = 0;
	STRPTR command    = (STRPTR)args[ARG_COMMAND];

	if(args[ARG_PROCESS] != NULL)
	    processNum = *(ULONG *)args[ARG_PROCESS];

	if(!full && !tcb && processNum == 0 && command == NULL)
	    all = TRUE;

	if(command != NULL)
	{
	    struct List     *cliList;
	    struct CLIInfo  *ci;

	    D(bug("command != NULL in Status\n"));

	    /* Get access to the rootnode */
	    ObtainSemaphore(&root->rn_RootLock);

	    D(bug("Got RootLock\n"));

	    cliList = (struct List *)&root->rn_CliList;
	    ci = (struct CLIInfo *)FindName(cliList, command);

	    if(ci != NULL)
	    {
		if(ci->ci_Process->pr_TaskNum != 0)
		{
		    printf(" %li\n", ci->ci_Process->pr_TaskNum);
		}
	    }
	    else
		retval = RETURN_WARN;

	    ReleaseSemaphore(&root->rn_RootLock);
	}
	else if(processNum != 0)
	{
	    struct Process *process;

	    ObtainSemaphore(&root->rn_RootLock);
	 
	    /* This is a temporary construction until I've fixed the
	       implementation of FindCliProc() */
	    Forbid();
	    process = FindCliProc(processNum);
	    Permit();

	    ReleaseSemaphore(&root->rn_RootLock);

	    if(process != NULL)
		printProcess(full, tcb, process);
	    else
		printf("Process %i does not exist\n", (int)processNum);
	}
	else
	{
	    struct List     *cliList;
	    struct CLIInfo  *ci;

	    ObtainSemaphore(&root->rn_RootLock);

	    D(bug("Got RootLock\n"));

	    cliList = (struct List *)&root->rn_CliList;

	    ForeachNode(cliList, (struct CLIInfo *)ci)
	    {
		printProcess(full, tcb, ci->ci_Process);
	    }

	    ReleaseSemaphore(&root->rn_RootLock);
	}

	FreeArgs(rda);
    }
    else
    {
        PrintFault(IoErr(), "Status");
        retval = RETURN_ERROR;
    }
    
    return retval;
}


/* Print the information for a certain cli process */
void printProcess(BOOL full, BOOL tcb, struct Process *process)
{
    struct CommandLineInterface *cli = BADDR(process->pr_CLI);

    /* This should never happen, I guess */
    if(cli == NULL)
	return;

    printf("Process %li ", process->pr_TaskNum);

    if(tcb || full)
    {
	printf("stk %lu, pri %lu ", (ULONG)cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT,
	       (ULONG)process->pr_Task.tc_Node.ln_Pri);
    }

    if(!tcb || full)
    {
	STRPTR  name;

	/* Sort of a hack. We rely on the fact that binary compatibility
	   BSTR:s is ended with a 0 byte */
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
	name = (STRPTR)cli->cli_CommandName + 1;
#else
	name = (STRPTR)cli->cli_CommandName;
#endif

	printf("Loaded as command: %s", name);
    }

    printf("\n");
}
