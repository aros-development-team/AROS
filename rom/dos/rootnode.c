/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Miscellaneous functions for dealing with DOS rootnode.
    Lang:
*/

#include <aros/debug.h>

#include <exec/lists.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include "dos_intern.h"


/* Add a CLI process to the RootNode structure. This is used by commands
   as C:Status and functions like dos.library/MaxCli() */
void addprocesstoroot(struct Process *process, struct DosLibrary *DOSBase)
{
    ULONG  *taskarray;
    ULONG  *newtaskarray;
    ULONG   size;
    ULONG   i;

    struct CommandLineInterface *cli =
	(struct CommandLineInterface *)BADDR(process->pr_CLI);

    struct RootNode *root = DOSBase->dl_Root;

    struct CLIInfo *ci;

    D(bug("Calling addprocesstoroot() with cli = %p\n", cli));

    if(cli == NULL)
	return;

    ci = (struct CLIInfo *)AllocVec(sizeof(struct CLIInfo), MEMF_PUBLIC);

    if(ci == NULL)
	return;

    ci->ci_Process = process;
  
    ObtainSemaphore(&root->rn_RootLock);

    /* Set the node's name to the process' name so we may use FindName()
       on the rn_CliList to locate a specific command */

    /* This is kind of hacky but doing it another way will be more
       troublesome; we rely here that even BSTR:s have a trailing 0. */

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    ci->ci_Node.ln_Name = ((STRPTR)cli->cli_CommandName) + 1;
#else
    ci->ci_Node.ln_Name = cli->cli_CommandName;
#endif
    
    /* Can't use AddTail() here as it complains about the list pointer */
    ADDTAIL((struct List *)&root->rn_CliList, (struct Node *)ci);

    taskarray = BADDR(root->rn_TaskArray);
    size = taskarray[0];
  
    /*
    ** Check out the taskarray for an empty slot
    */
    i = 1;
    
    while(i <= size)
    {
	if(0 == taskarray[i])
	{
	    taskarray[i] = (ULONG)&process->pr_MsgPort;
	    process->pr_TaskNum = i;

	    ReleaseSemaphore(&root->rn_RootLock);

	    D(bug("Returning from addprocesstoroot() -- 1\n"));
	    
	    return;
	}
	
	i++;
    }
    
    /*
    ** it seems like a new taskarray is needed 
    */
    newtaskarray = AllocMem(sizeof(ULONG) + (size + 1)*sizeof(APTR), MEMF_ANY);
    
    newtaskarray[0] = size + 1;
    i = 1;

    while(i <= size)
    {
	newtaskarray[i] = taskarray[i];
	i++;
    }

    newtaskarray[size + 1] = (ULONG)&process->pr_MsgPort;
    process->pr_TaskNum = size + 1;
    
    root->rn_TaskArray = MKBADDR(newtaskarray);

    FreeMem(taskarray, sizeof(ULONG) + size*sizeof(APTR));
    
    ReleaseSemaphore(&root->rn_RootLock);
}


void removefromrootnode(struct Process *process, struct DosLibrary *DOSBase)
{
    ULONG   size;
    ULONG  *taskarray;
    ULONG   i;

    struct Node     *temp;
    struct CLIInfo  *cliNode;
    struct RootNode *root = DOSBase->dl_Root;

    if (!__is_process(process) || process->pr_CLI == NULL)
    {
	return;
    }
    
    ObtainSemaphore(&root->rn_RootLock);
  
    /* Remove node from CliList */
    ForeachNodeSafe(&root->rn_CliList, cliNode, temp)
    {
	if (cliNode->ci_Process == process)
	{
	    Remove((struct Node *)cliNode);
	    FreeVec(cliNode);
	    break;
	}
    }

    taskarray = BADDR(root->rn_TaskArray);
    size = taskarray[0];
  
    i = 1;

    while (i <= size)
    {
	if (taskarray[i] == (ULONG)&process->pr_MsgPort)
	{
	    taskarray[i] = 0;
	    break;
	}

	i++;
    }
    
    ReleaseSemaphore(&root->rn_RootLock);
}
