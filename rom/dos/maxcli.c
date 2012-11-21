/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH0(ULONG, MaxCli,

/*  SYNOPSIS */

/*  LOCATION */
        struct DosLibrary *, DOSBase, 92, Dos)

/*  FUNCTION
        Returns the highest Cli number currently in use. Since processes
        may be added and removed at any time the returned value may already
        be wrong.

    INPUTS

    RESULT
        Maximum Cli number (_not_ the number of Clis).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    IPTR *taskarray = BADDR(DOSBase->dl_Root->rn_TaskArray);
    /* 
       The first IPTR in the taskarray contains the size of the
       taskarray = the max. number of processes the taskarray
       can currently hold. 
    */
    IPTR retval = taskarray[0];
    
    /* 
       Not all of the fields in the array may contain a valid
       pointer to a process and they might be NULL instead. So
       I search that array backwards until I find a valid entry.
    */
    while (retval && (IPTR)NULL != taskarray[retval])
        retval--;
    
    return retval;
    AROS_LIBFUNC_EXIT
} /* MaxCli */
