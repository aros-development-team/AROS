/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/dos.h>

#include <errno.h>

#include "__arosc_privdata.h"
#include "__filesystem_support.h"
#include "__stat.h"
#include "__upath.h"

/* like Dos_Lock() but no automatic soft link resolution */
static BPTR __lock(
    const char* name,
    LONG        accessMode);

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

    EXAMPLE

    BUGS

    SEE ALSO
        stat(), fstat()

    INTERNALS
	Consult stat() documentation for details.

******************************************************************************/
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    int res = 0;
    BPTR lock;

    /* check for empty path before potential conversion from "." to "" */
    if (aroscbase->acb_doupath && path && *path == '\0')
    {
        errno = ENOENT;
        return -1;
    }

    path = __path_u2a(path);
    if (path == NULL)
        return -1;

    lock = __lock(path, SHARED_LOCK);
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
            return __stat_from_path(path, sb);
        }

        errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }
    else
        res = __stat(lock, sb, FALSE);

    UnLock(lock);

    return res;
}

static BPTR __lock(
    const char* name,
    LONG        accessMode)
{
    return Lock(name, accessMode);
}
