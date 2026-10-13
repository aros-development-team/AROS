/*
    Copyright (C) 2025-2026, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fdopendir
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "__fdesc.h"
#include "__dirdesc.h"

/*****************************************************************************

    NAME */
#include <dirent.h>

        DIR *fdopendir(

/*  SYNOPSIS */
        int fd)

/*  FUNCTION
        Creates a directory stream for the directory referenced by the open
        file descriptor fd (from open() with O_DIRECTORY, openat(), or
        dirfd()). The descriptor is then owned by the stream and is closed
        by closedir().

    INPUTS
        fd - descriptor of an open directory

    RESULT
        A directory stream, or NULL with errno set (EBADF if fd is not
        open, ENOTDIR if it does not refer to a directory).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        opendir(), openat(), readdir(), closedir(), dirfd()

    INTERNALS
        Mirrors opendir() but reuses the descriptor's lock; the FIB is
        (re)examined so that readdir()'s ExNext() starts from the top.

******************************************************************************/
{
    DIR *dir;
    fdesc *desc;

    desc = __getfdesc(fd);
    if (!desc)
    {
        errno = EBADF;
        return NULL;
    }

    if (!(desc->fcb->privflags & _FCB_ISDIR))
    {
        errno = ENOTDIR;
        return NULL;
    }

    dir = malloc(sizeof(DIR));
    if (!dir)
    {
        errno = ENOMEM;
        return NULL;
    }

    dir->priv = AllocDosObject(DOS_FIB, NULL);
    if (!dir->priv)
    {
        errno = ENOMEM;
        free(dir);
        return NULL;
    }

    if (!Examine(desc->fcb->handle, dir->priv))
    {
        errno = __stdc_ioerr2errno(IoErr());
        FreeDosObject(DOS_FIB, dir->priv);
        free(dir);
        return NULL;
    }

    dir->fd = fd;
    dir->pos = 0;
    dir->ent.d_name[NAME_MAX] = '\0';

    D(bug("[posixc] %s(%d) = %p\n", __func__, fd, dir));
    return dir;
}
