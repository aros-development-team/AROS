/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>

#include <fcntl.h>
#include <errno.h>

#include "__errno.h"
#include "__open.h"

int fcntl(int fd, int cmd, int arg)
{
    GETUSER;
    fdesc *desc = __getfdesc(fd);

    if (!desc)
    {
	errno = EBADF;
	return -1;
    }

    switch (cmd)
    {
	case F_GETFL:
	    return desc->flags & (O_NONBLOCK|O_APPEND|O_ASYNC);

	case F_SETFL:
	    {
	        int oldmode = __oflags2amode(desc->flags & ~(O_NONBLOCK|O_APPEND|O_ASYNC));
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
