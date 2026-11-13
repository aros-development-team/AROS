/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: stdio internals
*/

#include "__posixc_intbase.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <exec/lists.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>
#include "__stdio.h"
#include "__fdesc.h"

int __smode2oflags(const char *mode)
{
    int ret = -1;
    int theresb = 0;

    switch (*mode++)
    {
        case 'r':
            ret = O_RDONLY;
            break;

        case 'w':
            ret = O_WRONLY | O_CREAT | O_TRUNC;
            break;

        case 'a':
            ret = O_WRONLY | O_CREAT | O_APPEND;
            break;

        default:
            errno = EINVAL;
            return -1;
    }

    if (*mode == 'b')
    {
        theresb = 1;
        mode++;
    }
    else if (*mode == 't')
    {
        // Silently ignore 't' (=text).
        // It's deprecated, but it's still in some sources,
        // and on other platforms they can compiled without problems.
        mode++;
    }

    if (*mode == '+')
    {
        ret = O_RDWR | (ret & ~O_ACCMODE);
        mode++;
    }

    if (*mode == 'b' && !theresb)
    {
        mode++;
    }

    if (*mode != '\0')
    {
        errno = EINVAL;
        return -1;
    }

    return ret;
}

int __oflags2sflags(int omode)
{
    int ret;

    switch (omode & O_ACCMODE)
    {
        case O_RDONLY:
            ret = __POSIXC_STDIO_READ;
            break;

        case O_WRONLY:
            ret = __POSIXC_STDIO_WRITE;
            break;

        case O_RDWR:
            ret = __POSIXC_STDIO_READ | __POSIXC_STDIO_WRITE;
            break;

        default:
            /* fds inherited across exec can carry unset accmode bits, and this
               runs from __init_stdio during PosixC OpenLib - before the
               opener's StdCBase is wired, so writing errno here would
               dereference a NULL _errno.  Don't veto std streams and don't
               touch errno: treat an unknown accmode as read+write. */
            ret = __POSIXC_STDIO_READ | __POSIXC_STDIO_WRITE;
            break;
    }

    if (omode & O_APPEND)
        ret |= __POSIXC_STDIO_APPEND;

    return ret;
}

FILE *__stdio_file(int fd, int oflags)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    FILENODE *fn = AllocPooled(PosixCBase->internalpool, sizeof(FILENODE));

    if (!fn)
        return NULL;
    AddTail((struct List *)&PosixCBase->stdio_files, (struct Node *)fn);
    fn->File.flags = __oflags2sflags(oflags);
    fn->File.fd = fd;
    return FILENODE2FILE(fn);
}

static FILE *__stdio_standard(int fd, int default_flags)
{
    fdesc *desc = __getfdesc(fd);

    /* A closed standard descriptor still has a FILE object, just as after
       close(0) in a running program. Do not reopen it or reject this opener.
       Later stdio calls report EBADF until the descriptor is reused. This
       hook runs before StdCBase is published, so fdopen's errno path is not
       available here. Preserve inherited access/append flags when present. */
    return __stdio_file(fd, desc ? desc->fcb->flags : default_flags);
}

int __init_stdio(struct PosixCIntBase *PosixCIntBase)
{
    struct PosixCBase *PosixCBase = (struct PosixCBase *)PosixCIntBase;
    NEWLIST(&PosixCIntBase->stdio_files);

    if
    (
        !(PosixCBase->_stdin  = __stdio_standard(STDIN_FILENO, O_RDONLY))  ||
        !(PosixCBase->_stdout = __stdio_standard(STDOUT_FILENO, O_WRONLY | O_APPEND)) ||
        !(PosixCBase->_stderr = __stdio_standard(STDERR_FILENO, O_WRONLY | O_APPEND))
    )
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return 0;
    }

    return 1;
}

ADD2OPENLIB(__init_stdio, 5);
