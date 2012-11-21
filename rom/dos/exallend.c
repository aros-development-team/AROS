/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH5(void, ExAllEnd,

/*  SYNOPSIS */
        AROS_LHA(BPTR,                  lock,    D1),
        AROS_LHA(struct ExAllData *,    buffer,  D2),
        AROS_LHA(LONG,                  size,    D3),
        AROS_LHA(LONG,                  data,    D4),
        AROS_LHA(struct ExAllControl *, control, D5),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 165, Dos)

/*  FUNCTION

    Stop an ExAll() operation before returning ERROR_NO_MORE_ENTRIES.

    INPUTS

    The inputs should correspond to the inputs for the ExAll() function.

    lock     --  lock on the directory that is being examined
    buffer   --  buffer for data returned
    size     --  size of 'buffer' in bytes
    type     --  type of data to be returned
    control  --  control data structure

    RESULT

    NOTES

    The control data structure must have been allocated with AllocDosObject().

    EXAMPLE

    BUGS

    SEE ALSO

    ExAll(), AllocDosObject()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (((struct InternalExAllControl *)control)->fib == NULL)
    {
        /* Get pointer to filehandle */
        struct FileLock *fl = (struct FileLock *)BADDR(lock);
        dopacket5(DOSBase, NULL, fl->fl_Task, ACTION_EXAMINE_ALL_END, (SIPTR)lock, (IPTR)buffer, (IPTR)size, (IPTR)data, (IPTR)control);
    }
    else
    {
        control->eac_LastKey = 0;
    }
        

    AROS_LIBFUNC_EXIT
} /* ExAllEnd */

