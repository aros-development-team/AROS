/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.
    Copyright (C) 2026 John Knipper.

    C99 function rename().
*/
#include <proto/dos.h>
#include <errno.h>


/*****************************************************************************

    NAME */
#include <stdio.h>

        int rename (

/*  SYNOPSIS */
        const char * oldpath,
        const char * newpath)

/*  FUNCTION
        Renames a file or directory.

    INPUTS
        oldpath - Complete path to existing file or directory.
        newpath - Complete path to the new file or directory.

    RESULT
        0 on success and -1 on error. In case of an error, errno is set.
        
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    LONG error;

    /* Request the native non-replacing operation. Handler guarantees are
     * qualified separately; this wrapper never deletes the destination. */
    if (Rename((CONST_STRPTR)oldpath, (CONST_STRPTR)newpath))
        return 0;

    error = IoErr();
    if (error == ERROR_OBJECT_EXISTS)
    {
        error = ERROR_ACTION_NOT_KNOWN;
    }

    /* Native replacement needs its own public capability contract. Refuse it
     * safely until available, and preserve the captured DOS error for callers. */
    errno = error == 0 ? EIO : __stdc_ioerr2errno(error);
    SetIoErr(error);
    return -1;
} /* rename */
