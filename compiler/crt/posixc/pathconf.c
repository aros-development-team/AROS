/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <limits.h>     /* _POSIX_*_MAX */
#include <stdio.h>      /* FILENAME_MAX */
#include <unistd.h>     /* _PC_*        */

/****************************************************************************
    NAME
        long pathconf(

    SYNOPSIS
        const char *path, int name)

    FUNCTION
        Return a fixed pathname limit selected by name. The
        filesystem containing path is not queried.

    INPUTS
        path - Ignored. The pointer is not dereferenced, and the
               pathname is neither resolved nor validated.
        name - _PC_PATH_MAX is the supported public query. See
               BUGS for the incorrectly implemented name-limit query.

    RESULT
        PATH_MAX (1024) for _PC_PATH_MAX. The implementation also
        returns FILENAME_MAX (256) for _POSIX_NAME_MAX (32). All
        other values return -1. errno is unchanged in every case.

    NOTES
        Include <unistd.h>. The result does not depend on whether
        path exists or on the filesystem that would resolve it.

    BUGS
        The name-limit branch tests _POSIX_NAME_MAX, a minimum
        limit constant, instead of the query selector _PC_NAME_MAX.
        _PC_NAME_MAX is not defined by the current public header.
        _PC_VDISABLE is declared but returns -1.

        A -1 result does not distinguish an unsupported selector
        from an indeterminate limit. The fixed positive results
        are not verified limits for the supplied pathname.

    SEE ALSO
        <unistd.h>, <limits.h>

*****************************************************************************/
long pathconf(const char *path, int name)
{
    /* TODO: Implement pathconf() properly */
    switch (name) {
    case _POSIX_NAME_MAX:
        return FILENAME_MAX;
    case _PC_PATH_MAX:
        return PATH_MAX;
    default:
        return -1;
    }
}
