/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function rename() with optional Amiga<>Posix file name conversion.
*/

#include <aros/debug.h>

#include <proto/dos.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "__upath.h"

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
          STRPTR aoldpath = (STRPTR)strdup((const char*)__path_u2a(oldpath));
    CONST_STRPTR anewpath = __path_u2a(newpath);
    int ret;

    /* __path_u2a has resolved paths like /toto/../a */
    if (anewpath[0] == '.')
    {
        if (anewpath[1] == '\0' || (anewpath[1] == '.' && anewpath[2] == '\0'))
        {
            errno = EEXIST;
            free(aoldpath);
            return -1;
        }
    }

    ret = __stdcio_rename(aoldpath, anewpath);

    free(aoldpath);

    return ret;
} /* rename */

