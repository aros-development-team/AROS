/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/dos.h>

#include <errno.h>

#include "__arosc_privdata.h"
#include "__upath.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int chown(

/*  SYNOPSIS */
	const char *path,
	uid_t      owner,
	gid_t      group)

/*  FUNCTION
        Change the user and group ownership of a file.
        
    INPUTS
        path  - the path to file
        owner - new owner ID
        group - new group ID

    RESULT
        0 on success and -1 on error. If an error occurred, the global
        variable errno is set.

    NOTES
        This implementation was done by looking at Olaf 'Olsen' Barthels
        clib2.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __GM_GetBase();
    int                  res     = -1;
    BPTR                 lock    = BNULL;
    struct FileInfoBlock *fib    = NULL;
    BOOL                 changed = TRUE;

    /* check for empty path before potential conversion from "." to "" */
    if (aroscbase->acb_doupath && path && *path == '\0')
    {
        errno = ENOENT;
        goto out;
    }

    path = __path_u2a(path);
    if (path == NULL)
        goto out;

    /* should some id not be changed */
    if (owner == -1 || group == -1)
    {
        if (!(fib = AllocDosObject(DOS_FIB, NULL)))
        {
            errno = __arosc_ioerr2errno(IoErr());
            goto out;
        }
    
        if (!(lock = Lock(path, SHARED_LOCK)) || !Examine(lock, fib))
        {
            errno = __arosc_ioerr2errno(IoErr());
            goto out;
        }

        /* set to previous id */
        if (owner == -1)
            owner = fib->fib_OwnerUID;

        if (group == -1)
            group = fib->fib_OwnerGID;

        if (owner == fib->fib_OwnerUID && group == fib->fib_OwnerGID)
            changed = FALSE;
    }
    
    if (owner > 65535 || group > 65535)
    {
        errno = EINVAL;
        goto out;
    }

    if (changed && !SetOwner(path, owner << 16 | group))
    {
        errno = __arosc_ioerr2errno(IoErr());
        goto out;
    }

    res = 0;

out:

    if (fib)
        FreeDosObject(DOS_FIB, fib);

    if (lock)
        UnLock(lock);

    return res;
}
