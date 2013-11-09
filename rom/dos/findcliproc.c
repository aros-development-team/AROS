/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Find a CLI process by number
    Lang: English
*/

#include <exec/lists.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(struct Process *, FindCliProc,

/*  SYNOPSIS */
        AROS_LHA(ULONG, num, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 91, Dos)

/*  FUNCTION
        Find a CLI process by its task number. The number must be greater
        than 0. 

    INPUTS
        num - The task number of the CLI to find.

    RESULT
        Pointer to the process if found, NULL otherwise.

    NOTES
        The process calling this function doesn't need to do any locking.

    EXAMPLE

    BUGS

    SEE ALSO
        Cli(), MaxCli()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RootNode *root = DOSBase->dl_Root;
    struct Process  *cliProc = NULL;
    struct CLIInfo *node;

    ObtainSemaphoreShared(&root->rn_RootLock);

    ForeachNode(&root->rn_CliList, node)
    {
        if (node->ci_Process->pr_TaskNum == num)
        {
            cliProc = node->ci_Process;
            break;
        }
    }

    ReleaseSemaphore(&root->rn_RootLock);
    return cliProc;
    
    AROS_LIBFUNC_EXIT
} /* FindCliProc */
