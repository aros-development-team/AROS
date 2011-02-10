/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI function freopen().
*/

#include <errno.h>

#include "__fdesc.h"
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	FILE *freopen (

/*  SYNOPSIS */
	const char *path,
	const char *mode,
	FILE       *stream
	)

/*  FUNCTION
	Opens the  file whose name is the string pointed to by path  and
	associates  the  stream  pointed to by stream with it.

    INPUTS
	path   - the file to open
	mode   - The mode of the stream  (same as with fopen()) must be com­
                 patible with the mode of the file  descriptor.   The  file
                 position  indicator  of  the  new  stream  is  set to that
                 belonging to fildes, and the error and end-of-file indica­
                 tors  are cleared.  Modes "w" or "w+" do not cause trunca­
                 tion of the file.  The file descriptor is not dup'ed,  and
                 will  be  closed  when  the  stream  created  by fdopen is
                 closed.
        stream - the stream to wich the file will be associated.

    RESULT
	NULL on error or stream.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	 open(), fclose(), fileno()

    INTERNALS

******************************************************************************/
{
    int fd, oflags;

    if (!(path && mode && stream))
    {
        errno = EFAULT;
	return NULL;
    }

    oflags = __smode2oflags(mode);
    fd = __open(stream->fd, path, oflags, 644);

    if (fd == -1)
        return NULL;

    stream->flags = __oflags2sflags(oflags);

    return stream;
}
