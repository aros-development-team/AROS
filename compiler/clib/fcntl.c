/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>

#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>

#include "__errno.h"
#include "__open.h"

int fcntl(int fd, int cmd, ...)
{
    fdesc *desc = __getfdesc(fd);

    if (!desc)
    {
        errno = EBADF;
        return -1;
    }

    switch (cmd)
    {
        case F_DUPFD:
        {
            va_list ap;
            int     arg;
            
            va_start(ap, cmd);
            arg = va_arg(ap, int);
            va_end(ap);
            
            /*
                FIXME: FD_CLOEXEC must be off on the copy, once this flag
                       is supported (related to F_GETFD and F_SETFD).
            */
            
            return dup2(fd, __getfirstfd(arg));
        }
	
        case F_GETFL:
            return desc->flags & (O_NONBLOCK|O_APPEND|O_ASYNC);

        case F_SETFL:
        {
            va_list ap;
            int     arg;
            int     oldmode = __oflags2amode(desc->flags & ~(O_NONBLOCK|O_APPEND|O_ASYNC));

            va_start(ap, cmd);
            arg = va_arg(ap, int);
            va_end(ap);
  
            arg &= (O_NONBLOCK|O_APPEND|O_ASYNC);

            if (ChangeMode(CHANGE_FH, desc->fh, oldmode | __oflags2amode(arg)) == DOSTRUE)
            {
                desc->flags &= ~(O_NONBLOCK|O_APPEND|O_ASYNC);
                desc->flags |= arg;
                return 0;
            }

            errno = IoErr2errno(IoErr());
            return -1;
        }

        default:
            errno = EINVAL;
            return -1;
    }
}
