/*
    (C) 1995-2000 AROS - The Amiga Research OS
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

    HISTORY

    02.12.2000  SDuvan  --  rewrote to use rootnode rn_TaskArray instead of
                            hacking the process lists

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct RootNode *root = DOSBase->dl_Root;
    struct Process  *cliProc = NULL;

    ObtainSemaphoreShared(&root->rn_RootLock);

    /* This is quite ugly, I know, but the structure of the rn_TaskArray
       is just brain damaged! We just pick the 'num':th process of the
       array and return it in case 'num' _might_ exist */
    if (num < *(ULONG *)BADDR(root->rn_TaskArray))
    {
	cliProc = ((struct Process **)BADDR(root->rn_TaskArray))[num];
    }

    ReleaseSemaphore(&root->rn_RootLock);

    return cliProc;
    
    AROS_LIBFUNC_EXIT
} /* FindCliProc */
