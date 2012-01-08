/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>

#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <fcntl.h>

	int fcntl(

/*  SYNOPSIS */
	int fd,
	int cmd,
	...)

/*  FUNCTION
	Perform operation specified in cmd on the file descriptor fd.
	Some operations require additional arguments, in this case they
	follow the cmd argument. The following operations are available:
	
	F_DUPFD (int)  - Duplicate file descriptor fd as the lowest numbered
	                 file descriptor greater or equal to the operation
	                 argument.

	F_GETFD (void) - Read the file descriptor flags

	F_SETFD (int)  - Set the file descriptor flags to value given in
	                 the operation argument

	F_GETFL (void) - Read the file status flags

	F_SETFL (int)  - Set the file status flags to value given in the 
	                 operation argument.

	File descriptor flags are zero or more ORed constants:
	
	FD_CLOEXEC - File descriptor will be closed during execve()
	
	File descriptor flags are not copied during duplication of file
	descriptors.
	
	File status flags are the flags given as mode parameter to open()
	function call. You can change only a few file status flags in opened
	file descriptor: O_NONBLOCK, O_APPEND and O_ASYNC. Any other file
	status flags passed in F_SETFL argument will be ignored.
	
	All duplicated file descriptors share the same set of file status
	flags.

    INPUTS
	fd - File descriptor to perform operation on.
	cmd - Operation specifier.
	... - Operation arguments.

    RESULT
	The return value of the function depends on the performed operation:

	F_DUPFD  - New duplicated file descriptor

	F_GETFD  - File descriptor flags
	
	F_SETFD  - 0

	F_GETFL  - File status flags

	F_SETFL  - 0 on success, -1 on error. In case of error a global errno
		   variable is set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	open()

    INTERNALS

******************************************************************************/
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
        case F_GETFD:
            return desc->fdflags;
            
        case F_SETFD:
        {
            va_list ap;
            int     arg;

            va_start(ap, cmd);
            arg = va_arg(ap, int);
            va_end(ap);

            desc->fdflags = arg;
            return 0;
        }
	
        case F_GETFL:
            return desc->fcb->flags & (O_NONBLOCK|O_APPEND|O_ASYNC);

        case F_SETFL:
        {
            va_list ap;
            int     arg;
            int     oldmode = __oflags2amode(desc->fcb->flags & ~(O_NONBLOCK|O_APPEND|O_ASYNC));

            va_start(ap, cmd);
            arg = va_arg(ap, int);
            va_end(ap);
  
            arg &= (O_NONBLOCK|O_APPEND|O_ASYNC);

            if (ChangeMode(CHANGE_FH, desc->fcb->fh, oldmode | __oflags2amode(arg)) == DOSTRUE)
            {
                desc->fcb->flags &= ~(O_NONBLOCK|O_APPEND|O_ASYNC);
                desc->fcb->flags |= arg;
                return 0;
            }

            errno = __arosc_ioerr2errno(IoErr());
            return -1;
        }

        default:
            errno = EINVAL;
            return -1;
    }
}
