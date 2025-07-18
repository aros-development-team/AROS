/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function remove() with optional Amiga<>Posix file name conversion.
*/

#include <proto/dos.h>
#include <errno.h>
#include "__upath.h"

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
    return __stdcio_remove(__path_u2a(pathname));
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
