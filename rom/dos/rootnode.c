/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
    IPTR  *taskarray;
    IPTR  *newtaskarray;
    ULONG  size;
    ULONG  i;

    struct CommandLineInterface *cli =
        (struct CommandLineInterface *)BADDR(process->pr_CLI);

    struct RootNode *root = DOSBase->dl_Root;

    struct CLIInfo *ci;

    D(bug("Calling addprocesstoroot(%p) with cli = %p\n", process, cli));

    if(cli == NULL)
        return;

    ci = (struct CLIInfo *)AllocVec(sizeof(struct CLIInfo), MEMF_PUBLIC);

    if(ci == NULL)
        return;

    ObtainSemaphore(&root->rn_RootLock);

    D(bug("[addprocesstoroot] Adding to CliList\n"));
    /* Set the node's name to the process' name so we may use FindName()
       on the rn_CliList to locate a specific command */

    /* This is kind of hacky but doing it another way will be more
       troublesome; we rely here that even BSTR:s have a trailing 0. */

    ci->ci_Process = process;
    ci->ci_Node.ln_Name = AROS_BSTR_ADDR(cli->cli_CommandName);
    
    /* Can't use AddTail() here as it complains about the list pointer */
    ADDTAIL((struct List *)&root->rn_CliList, (struct Node *)ci);

    D(bug("[addprocesstoroot] Adding to TaskArray\n"));
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
            taskarray[i] = (IPTR)&process->pr_MsgPort;
            process->pr_TaskNum = i;
            goto done;
        }
        
        i++;
    }
    
    /*
    ** it seems like a new taskarray is needed 
    */
    newtaskarray = AllocMem(sizeof(IPTR) + (size + 1)*sizeof(APTR), MEMF_ANY);
    
    newtaskarray[0] = size + 1;
    i = 1;

    while(i <= size)
    {
        newtaskarray[i] = taskarray[i];
        i++;
    }

    newtaskarray[size + 1] = (IPTR)&process->pr_MsgPort;
    process->pr_TaskNum = size + 1;
    
    root->rn_TaskArray = MKBADDR(newtaskarray);

    FreeMem(taskarray, sizeof(IPTR) + size*sizeof(APTR));
   
done:
    D(bug("Returning from addprocesstoroot() (%d)\n", process->pr_TaskNum));
    ReleaseSemaphore(&root->rn_RootLock);
}


void removefromrootnode(struct Process *process, struct DosLibrary *DOSBase)
{
    IPTR  *taskarray;
    struct CLIInfo *ci, *tmp;

    struct RootNode *root = DOSBase->dl_Root;

    D(bug("[removefromrootnode] %p, TaskNum %d\n", process, process->pr_TaskNum));

    if (!__is_process(process) || process->pr_CLI == BNULL)
    {
        D(bug("[removefromrootnode] Strange. Doesn't seem be a CLI...\n"));
        return;
    }
    
    ObtainSemaphore(&root->rn_RootLock);

    ForeachNodeSafe(&root->rn_CliList, ci, tmp) {
        if (ci->ci_Process == process) {
            D(bug("[removefromrootnode] Removing from CLIList\n"));
            Remove((struct Node *)ci);
            FreeVec(ci);
        }
    }

    D(bug("[removefromrootnode] Removing from TaskArray\n"));
    taskarray = BADDR(root->rn_TaskArray);
    taskarray[process->pr_TaskNum] = 0;
        
    ReleaseSemaphore(&root->rn_RootLock);
}
