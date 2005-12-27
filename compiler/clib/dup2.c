/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function dup2().
*/

#include <errno.h>
#include "__open.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int dup2 (

/*  SYNOPSIS */
	int oldfd,
	int newfd
	)

/*  FUNCTION
	Duplicates a file descriptor.

	The object referenced by the descriptor does not distinguish between oldd
        and newd in any way.  Thus if newd and oldd are duplicate references to
        an open file, read(),  write() and lseek() calls all move a single
        pointer into the file, and append mode, non-blocking I/O and asynchronous
        I/O options are shared between the references.  If a separate pointer
        into the file is desired, a different object reference to the file must be
        obtained by issuing an additional open(2) call.  The close-on-exec flag
        on the new file descriptor is unset.

    INPUTS
	oldfd - The file descriptor to be duplicated
	newfd - The value of the new descriptor we want the old one to be duplicated in

    RESULT
	-1 for error or newfd on success

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	 accept(),  open(),  close(),  fcntl(),  pipe(),  socket()

    INTERNALS

******************************************************************************/
{
    fdesc *oldfdesc;

    oldfdesc = __getfdesc(oldfd);
    if (!oldfdesc)
    {
    	errno = EBADF;
	return -1;
    }

    newfd =__getfdslot(newfd);
    if (newfd != -1)
    {
	oldfdesc->opencount++;
	__setfdesc(newfd, oldfdesc);
    }

    return newfd;
}

