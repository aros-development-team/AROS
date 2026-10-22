/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.
    Copyright (C) 2026 John Knipper.

    C99 function remove() with optional Amiga<>Posix file name conversion.
*/

#include <proto/dos.h>
#include <errno.h>
#include "__upath.h"
#include "__dos_error.h"

extern int __stdcio_remove(const char *);

/*****************************************************************************

    NAME */
#include <stdio.h>

int remove (

/*  SYNOPSIS */
        const char * pathname)

/*  FUNCTION
        Deletes a file or directory.

    INPUTS
        pathname - Complete path to the file or directory.

    RESULT
        0 on success and -1 on error. In case of an error, errno is set.
        
    NOTES
        Identical to unlink

    EXAMPLE

    BUGS

    SEE ALSO
        unlink()

    INTERNALS
        Uses stdcio.library remove() function after path name conversion

******************************************************************************/
{
    const char *native_path = __path_u2a(pathname);
    LONG error;
    int result;

    if (native_path == NULL)
        return -1;
    result = __stdcio_remove(native_path);
    if (result != 0)
    {
        error = IoErr();
        errno = __posixc_dos_error(error);
        SetIoErr(error);
    }
    return result;
} /* remove */


/*****************************************************************************

    NAME
#include <unistd.h>

        int unlink (

    SYNOPSIS
        const char * pathname)

    FUNCTION
        Delete a file from disk.

    INPUTS
        pathname - Complete path to the file

    RESULT
        0 on success and -1 on error. In case of an error, errno is set.

    NOTES
        Identical to remove

    EXAMPLE
        // Delete the file xyz in the current directory
        unlink ("xyz");

    BUGS

    SEE ALSO
        remove()

    INTERNALS

******************************************************************************/
