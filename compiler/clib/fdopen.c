/*
    Copyrigth 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: POSIX function fdopen()
    Lang: english
*/


#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <exec/lists.h>
#include <proto/exec.h>

#include "__stdio.h"
#include "__open.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	FILE *fdopen (

/*  SYNOPSIS */
	int         filedes,
	const char *mode
	)

/*  FUNCTION
	function associates a stream with an existing file descriptor.

    INPUTS
	filedes - The descriptor the stream has to be associated with
	mode    - The mode of the stream  (same as with fopen()) must be com­
                  patible with the mode of the file  descriptor.   The  file
                  position  indicator  of  the  new  stream  is  set to that
                  belonging to fildes, and the error and end-of-file indica­
                  tors  are cleared.  Modes "w" or "w+" do not cause trunca­
                  tion of the file.  The file descriptor is not dup'ed,  and
                  will  be  closed  when  the  stream  created  by fdopen is
                  closed.

    RESULT
	NULL on error or the new stream assiciated with the descriptor.

	The new descriptor returned by the call is the lowest numbered
	descriptor currently not in use by the process.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	 open(), fclose(), fileno()

    INTERNALS

    HISTORY
	27.04.2001 falemagn created

******************************************************************************/
{
    int sflags, oflags;
    fdesc *fdesc;
    FILENODE *fn;

    if (!(fdesc = __getfdesc(filedes)))
    {
	errno = EBADF;
	return NULL;
    }


    oflags = fdesc->flags;

    if (mode)
    {
	int tmp;

	oflags = __smode2oflags(mode);
	tmp = oflags & O_ACCMODE;

	/* check if oflags are a subset of the flags that the already open file has */
	if (tmp != O_RDWR && (tmp != (fdesc->flags & O_ACCMODE)))
	{
	    errno = EINVAL;
	    return NULL;
	}
    }

    sflags = __oflags2sflags(oflags);

    fn = malloc(sizeof(FILENODE));
    if (!fn) return NULL;

    fn->File.flags = sflags;
    fn->File.fd = filedes;

    AddTail ((struct List *)&__stdio_files, (struct Node *)fn);

    return FILENODE2FILE(fn);
}
