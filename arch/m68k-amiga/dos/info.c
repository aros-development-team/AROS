/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: info.c 30792 2009-03-07 22:40:04Z neil $

    Desc:
    Lang: English
*/
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */

#include <proto/dos.h>
#include <dos/filesystem.h>

	AROS_LH2(LONG, Info,

/*  SYNOPSIS */
	AROS_LHA(BPTR             , lock, D1),
	AROS_LHA(struct InfoData *, parameterBlock, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 19, Dos)

/*  FUNCTION

    Get information about a volume in the system.

    INPUTS

    lock            --  a lock on any file on the volume for which information
                        should be supplied
    parameterBlock  --  pointer to an InfoData structure

    RESULT

    Boolean indicating success or failure. If TRUE (success) the
    'parameterBlock' is filled with information on the volume.

    NOTES

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

    status = dopacket2(DOSBase, NULL, fl->fl_Task, ACTION_INFO, lock, MKBADDR(parameterBlock));
    return status;

    AROS_LIBFUNC_EXIT
} /* Info */
