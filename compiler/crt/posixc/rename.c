/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.
    Copyright (C) 2026 John Knipper.

    C99 function rename() with optional Amiga<>Posix file name conversion.
*/

#include <aros/debug.h>

#include <proto/dos.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "__upath.h"
#include "__dos_error.h"

extern int __stdcio_rename(const char *, const char *);

/*****************************************************************************

    NAME */
#include <stdio.h>

int rename (

/*  SYNOPSIS */
        const char * oldpath,
        const char * newpath
        )

/*  FUNCTION
        Changes the name or location of a file or directory from `oldpath` to `newpath`.

    INPUTS
        oldpath - The current path to an existing file or directory.
        newpath - The new desired path for the file or directory.

    RESULT
        Returns 0 on success.
        Returns -1 on failure and sets errno accordingly.

    NOTES
        - The function performs path conversions for platform compatibility.
        - Paths with relative elements like '.' or '..' are handled.
        - If `newpath` resolves to "." or ".." alone, the function returns
          an error (EEXIST).

    EXAMPLE
        if (rename("/home/user/oldfile.txt", "/home/user/newfile.txt") != 0) {
            perror("rename failed");
        }

    BUGS
        None known.

    SEE ALSO
        stdio.h rename(), unlink(), mkdir()

    INTERNALS
        Uses __path_u2a() to convert paths to Amiga-compatible strings.
        Internally calls stdcio.library's rename() to perform the actual renaming
        after path conversion.

******************************************************************************/
{
    const char *converted_old = __path_u2a(oldpath);
    char *aoldpath;
    const char *anewpath;
    LONG error;
    int ret, saved_errno;

    if (converted_old == NULL)
        return -1;
    aoldpath = strdup(converted_old);
    if (aoldpath == NULL)
    {
        errno = ENOMEM;
        return -1;
    }
    anewpath = __path_u2a(newpath);
    if (anewpath == NULL)
    {
        saved_errno = errno;
        free(aoldpath);
        errno = saved_errno;
        return -1;
    }

    /* __path_u2a has resolved paths like /toto/../a */
    if (anewpath[0] == '.')
    {
        if (anewpath[1] == '\0' || (anewpath[1] == '.' && anewpath[2] == '\0'))
        {
            free(aoldpath);
            errno = EEXIST;
            return -1;
        }
    }

    ret = __stdcio_rename(aoldpath, anewpath);
    error = ret != 0 ? IoErr() : 0;

    free(aoldpath);
    if (ret != 0)
    {
        /* Cleanup and the delegated StdCIO call must not choose the error
         * observed by the calling POSIX/Rust context. */
        errno = __posixc_dos_error(error);
        SetIoErr(error);
    }
    return ret;
} /* rename */

