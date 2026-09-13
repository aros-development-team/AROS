/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: helpers for the POSIX.1-2008 *at() functions
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "__fdesc.h"
#include "__at.h"

int __at_isabsolute(const char *pathname)
{
    const char *colon, *slash;

    if (!pathname)
        return 0;
    if (pathname[0] == '/')
        return 1;
    /* "Volume:" or "Assign:" - a colon before any directory separator */
    colon = strchr(pathname, ':');
    slash = strchr(pathname, '/');
    return colon != NULL && (slash == NULL || colon < slash);
}

int __at_enter(int dirfd, BPTR *oldcd, BPTR *dirlock)
{
    fdesc *desc;

    *oldcd = BNULL;
    *dirlock = BNULL;

    if (dirfd == AT_FDCWD)
        return 0;

    desc = __getfdesc(dirfd);
    if (!desc)
    {
        errno = EBADF;
        return -1;
    }
    if (!(desc->fcb->privflags & _FCB_ISDIR))
    {
        errno = ENOTDIR;
        return -1;
    }

    /* The directory descriptor holds a shared lock; duplicate it so the
       current directory stays valid even if the descriptor is closed by
       another thread meanwhile. */
    *dirlock = DupLock(desc->fcb->handle);
    if (*dirlock == BNULL)
    {
        errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }

    *oldcd = CurrentDir(*dirlock);
    D(bug("[posixc] %s: dirfd %d -> lock %p (old cd %p)\n", __func__, dirfd, BADDR(*dirlock), BADDR(*oldcd)));
    return 0;
}

void __at_leave(BPTR oldcd, BPTR dirlock)
{
    if (dirlock != BNULL)
    {
        CurrentDir(oldcd);
        UnLock(dirlock);
    }
}
