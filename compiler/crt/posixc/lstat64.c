/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>

#include <errno.h>

#include "__posixc_intbase.h"
#include "__stat.h"
#include "__upath.h"

/*****************************************************************************

    NAME */

#include <sys/stat.h>

        int lstat64(

/*  SYNOPSIS */
        const char  *path,
        struct stat64 *sb)

/*  FUNCTION
        Returns information about a file like stat does except that lstat
        does not follow symbolic links. Information is stored in stat
        structure. Consult stat() documentation for detailed description
        of that structure.

    INPUTS
        path - Pathname of the file
        sb - Pointer to stat structure that will be filled by the lstat() call.

    RESULT
        0 on success and -1 on error. If an error occurred, the global
        variable errno is set.

    NOTES
        Only the final component of path is exempt from soft link
        resolution. A path that names a root or ends in a separator has
        no such component and is resolved like stat().

    EXAMPLE

    BUGS

    SEE ALSO
        stat64(), fstat64(), readlink()

    INTERNALS
        dos.library's Lock() resolves soft links, so the path is first
        probed with ReadLink(). Only a soft link is then looked up as an
        entry of its parent directory by __stat64_from_path(); any other
        object is described through its lock, exactly as stat64() does.
        Consult stat() documentation for details.

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    int res;
    BPTR lock;

    /* check for empty path before potential conversion from "." to "" */
    if (PosixCBase->doupath && path && *path == '\0')
    {
        errno = ENOENT;
        return -1;
    }

    path = __path_u2a(path);
    if (path == NULL)
        return -1;

    /* A root or a trailing separator leaves no final component to preserve */
    if (*FilePart((CONST_STRPTR)path) != '\0' && __path_is_softlink(path))
    {
        /* Lock() would follow a soft link to its target: examine the
           entry in its parent directory instead. Keep that answer only
           for a link; an ordinary object reached through a linked
           directory is described through its lock, as stat() does. */
        if (__stat64_from_path(path, sb) == 0 && S_ISLNK(sb->st_mode))
            return 0;
    }

    lock = Lock(path, SHARED_LOCK);
    if (!lock)
    {
        if (   IoErr() == ERROR_IS_SOFT_LINK
            || IoErr() == ERROR_OBJECT_IN_USE)
        {
            /* either the file is already locked exclusively
               or it is a soft link, in both cases only way
               to get info about it is to find it in the
               parent directory with the ExNext() function
            */

            SetIoErr(0);
            return __stat64_from_path(path, sb);
        }

        errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }

    res = __stat64(lock, sb, FALSE);

    UnLock(lock);

    return res;
}
