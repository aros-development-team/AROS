/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <dos/dosextens.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(LONG, Flush,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 60, Dos)

/*  FUNCTION
        Flushes any pending writes on the file. If the file was used
        for input and there is still some data to read it tries to
        seek back to the expected position.

    INPUTS
        file - filehandle

    RESULT
        != 0 on success, 0 on error. IoErr() gives additional information
        in that case.

    NOTES
        On AROS calling Flush() from different tasks on the same file handle
        is serialised. This means that most of the time it is possible to
        do I/O in one task to a file handle where Flush() is being called
        in another task on that file handle.
        No multi-thread safety is guaranteed though and data may be lost if
        I/O is done in parallel from different tasks on the same file handle.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle. */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    if (!fh)
        return DOSTRUE;

    /* The file must be in write mode. */
    if( fh->fh_Flags & FHF_WRITE )
    {
        /* Handle append mode. */
        if( fh->fh_Flags & FHF_APPEND )
        {
            InternalSeek( fh, 0, OFFSET_END, DOSBase );
        }
        
        return InternalFlush( fh, DOSBase );
    }
    else if( fh->fh_Pos < fh->fh_End )
    {
        int offset = fh->fh_Pos - fh->fh_End;
        
        fh->fh_Pos = fh->fh_End = 0;
        
        /* Read mode. Try to seek back to the current position. */
        if( InternalSeek( fh, offset, OFFSET_CURRENT, DOSBase ) < 0 )
        {
            return FALSE;
        }
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* Flush */
