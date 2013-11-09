/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Lock the directory a file is located in
    Lang: english
*/

#include <aros/debug.h>
#include "dos_intern.h"
#include <dos/dosasl.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(BPTR, ParentOfFH,

/*  SYNOPSIS */
        AROS_LHA(BPTR, fh, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 64, Dos)

/*  FUNCTION
        Lock the directory a file is located in.

    INPUTS
        fh   - Filehandle of which you want to obtain the parent

    RESULT
        lock - Lock on the parent directory of the filehandle or
               NULL for failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        Lock(), UnLock(), ParentDir() 

    INTERNALS

*****************************************************************************/
{  
    AROS_LIBFUNC_INIT

    struct FileHandle *handle = BADDR(fh);
    D(bug("[ParentOfFH] fh=%x\n", handle));
    return (BPTR)dopacket1(DOSBase, NULL, handle->fh_Type, ACTION_PARENT_FH, handle->fh_Arg1);

    AROS_LIBFUNC_EXIT
  
} /* ParentOfFH */
