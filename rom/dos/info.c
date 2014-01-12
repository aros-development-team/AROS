/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */

#include <proto/dos.h>

        AROS_LH2(LONG, Info,

/*  SYNOPSIS */
        AROS_LHA(BPTR             , lock, D1),
        AROS_LHA(struct InfoData *, parameterBlock, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 19, Dos)

/*  FUNCTION
        Get information about a volume in the system.

    INPUTS
        lock           - a lock on any file on the volume for which information
                         should be supplied, or 0
        parameterBlock - pointer to an InfoData structure

    RESULT
        Boolean indicating success or failure. If TRUE (success) the
        'parameterBlock' is filled with information on the volume.

    NOTES
        Supplying a lock of 0 will return InfoData from the task that is
        returned from GetFileSysTask() (usually the boot volume's filesystem
        "SYS:").
        
    EXAMPLE

    BUGS

    SEE ALSO
        <dos/dos.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileLock *fl = (struct FileLock *)BADDR(lock);
    LONG status;

    if (fl && fl->fl_Task == NULL)
    {
        /* Special case for NIL: */
        status = 0;
    }
    else
        status = dopacket2(DOSBase, NULL, fl ? fl->fl_Task : GetFileSysTask(), ACTION_INFO, lock, MKBADDR(parameterBlock));
    return status;

    AROS_LIBFUNC_EXIT
} /* Info */
