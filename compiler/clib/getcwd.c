/*
    Copyright (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function getcwd()
    Lang: english
*/

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	char *getcwd (

/*  SYNOPSIS */
	char *buf,
	size_t size)

/*  FUNCTION
	Get the current working directory.

    INPUTS
	buf - Pointer of the buffer where the path is to be stored
	size - The size of the above buffer

    RESULT
	Copies the absolute pathname of the current working directory
	to the buffer. If the pathname is longer than the buffer
	(with lenght "size") NULL is returned and errno set to ERANGE.
	Otherwise the pointer to the buffer is returned.

    NOTES
	If buf is NULL this function will allocate the buffer itself
	using malloc() and the specified size "size". If size is
	0, too, the buffer is allocated to hold the whole path.
	It is possible and recommended to free() this buffer yourself!

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    GETUSER;

    char pathname[FILENAME_MAX];

    if (GetCurrentDirName (pathname, FILENAME_MAX) == 0)
    {
	errno = IoErr2errno (IoErr ());
	return NULL;
    }

    if (buf != NULL)
    {
	if (strlen(pathname) < size)
	{
	    strcpy (buf, pathname);
	}
	else
	{
	    errno = ERANGE;
	    return NULL;
	}
    }
    else
    {
	int len;
	char *newbuf;

	len = strlen(pathname);

	if (size == 0)
	{
	    size = len+1;
	}

	if (len < size)
	{
	    newbuf = (char *)malloc (size*sizeof(char));
	    strcpy (newbuf, pathname);
	    return newbuf;
	}
	else
	{
	    errno = ERANGE;
	    return NULL;
	}
    }

    return buf;

} /* getcwd */

