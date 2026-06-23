/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function getgrgid_r().
*/

#include <errno.h>

#include "__pwdgrp.h"

/*****************************************************************************

    NAME */

#include <grp.h>

        int getgrgid_r(

/*  SYNOPSIS */
        gid_t gid,
        struct group *grp,
        char *buffer,
        size_t bufsize,
        struct group **result)

/*  FUNCTION
        Reentrant version of getgrgid(). Looks up a group database entry by
        group ID and stores the result in caller-provided storage.

    INPUTS
        gid     - the group ID to look up.
        grp     - caller-provided struct group to fill in.
        buffer  - caller-provided storage for the strings and member list
                  pointed to by grp.
        bufsize - the size of buffer in bytes.
        result  - on success set to grp, or to NULL when no entry was found.

    RESULT
        0 on success (including "not found", with *result set to NULL).
        On error a positive error number is returned (ERANGE if buffer was
        too small, or e.g. ENOSYS if the group database is unavailable).

    NOTES
        Unlike getgrgid(), the returned data is stored in caller-provided
        memory and is not overwritten by a subsequent lookup.

    EXAMPLE

    BUGS

    SEE ALSO
        getgrgid(), getgrnam_r()

    INTERNALS
        Wraps getgrgid() and deep-copies its result into the caller buffer.

******************************************************************************/
{
    struct group *g;
    int saved_errno = errno;
    int ret;

    if (result)
        *result = NULL;

    if (!grp || !buffer || !result)
        return EINVAL;

    errno = 0;
    g = getgrgid(gid);
    if (!g)
    {
        /* errno == 0 means "not found": return 0 with *result == NULL. */
        ret = errno;
        errno = saved_errno;
        return ret;
    }

    ret = __posix_grp_to_buf(g, grp, buffer, bufsize);
    errno = saved_errno;

    if (ret == 0)
        *result = grp;

    return ret;
}
