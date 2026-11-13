/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function getpwnam_r().
*/

#include <errno.h>

#include "__pwdgrp.h"

/*****************************************************************************

    NAME */

#include <pwd.h>

        int getpwnam_r(

/*  SYNOPSIS */
        const char *name,
        struct passwd *pwd,
        char *buffer,
        size_t bufsize,
        struct passwd **result)

/*  FUNCTION
        Reentrant version of getpwnam(). Looks up a user database entry by
        name and stores the result in caller-provided storage.

    INPUTS
        name    - the username to look up.
        pwd     - caller-provided struct passwd to fill in.
        buffer  - caller-provided storage for the strings pointed to by pwd.
        bufsize - the size of buffer in bytes.
        result  - on success set to pwd, or to NULL when no entry was found.

    RESULT
        0 on success (including "not found", with *result set to NULL).
        On error a positive error number is returned (ERANGE if buffer was
        too small, or e.g. ENOSYS if the user database is unavailable).

    NOTES
        Unlike getpwnam(), the returned data is stored in caller-provided
        memory and is not overwritten by a subsequent lookup.

    EXAMPLE

    BUGS

    SEE ALSO
        getpwnam(), getpwuid_r()

    INTERNALS
        Wraps getpwnam() and deep-copies its result into the caller buffer.

******************************************************************************/
{
    struct passwd *p;
    int saved_errno = errno;
    int ret;

    if (result)
        *result = NULL;

    if (!pwd || !buffer || !result)
        return EINVAL;

    errno = 0;
    p = getpwnam(name);
    if (!p)
    {
        /* errno == 0 means "not found": return 0 with *result == NULL. */
        ret = errno;
        errno = saved_errno;
        return ret;
    }

    ret = __posix_pwd_to_buf(p, pwd, buffer, bufsize);
    errno = saved_errno;

    if (ret == 0)
        *result = pwd;

    return ret;
}
