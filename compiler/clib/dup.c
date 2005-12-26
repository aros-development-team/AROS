/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function dup().
*/

#include <errno.h>
#include "__open.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int dup (

/*  SYNOPSIS */
	int oldfd
	)

/*  FUNCTION
	Duplicates a file descriptor.

	The object referenced by the descriptor does not distinguish between oldd
        and newd in any way.  Thus if newd and oldd are duplicate references to
        an open file, read(),  write() and lseek() calls all move a single
        pointer into the file, and append mode, non-blocking I/O and asynchronous
        I/O options are shared between the references.  If a separate pointer in­
        to the file is desired, a different object reference to the file must be
        obtained by issuing an additional open(2) call.  The close-on-exec flag
        on the new file descriptor is unset.

    INPUTS
	oldfd - The file descriptor to be duplicated

    RESULT
	-1 for error or the new descriptor.

	The new descriptor returned by the call is the lowest numbered
	descriptor currently not in use by the process.

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
    return dup2(oldfd, __getfirstfd(0));
}

