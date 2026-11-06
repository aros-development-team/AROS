/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>

#include <errno.h>

#include "__posixc_intbase.h"
#include "__stat.h"
#include "__upath.h"

/*****************************************************************************

    NAME */

#include <sys/stat.h>

        int lstat(

/*  SYNOPSIS */
        const char  *path,
        struct stat *sb)

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
        stat(), fstat(), readlink()

    INTERNALS
        dos.library's Lock() resolves soft links, so the final component is
        looked up as an entry of its parent directory by __stat_from_path()
        instead. Consult stat() documentation for details.

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

    if (*FilePart((CONST_STRPTR)path) != '\0')
    {
        /* Lock() would follow a soft link to its target: examine the
           entry in its parent directory instead. */
        return __stat_from_path(path, sb);
    }

    /* A root, or a trailing separator: no final component to preserve */
    lock = Lock(path, SHARED_LOCK);
    if (!lock)
    {
        errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }

    res = __stat(lock, sb, FALSE);

    UnLock(lock);

    return res;
}
