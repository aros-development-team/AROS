/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function fileno().
*/

#include <errno.h>
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fileno (

/*  SYNOPSIS */
	FILE *stream)

/*  FUNCTION
	Returns the descriptor associated with the stream

    INPUTS
	strem - the stream to get the descriptor from

    RESULT
	The integer descriptor

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	open()

    INTERNALS

******************************************************************************/
{
    if (!stream) /* safety check */
    {
    	errno = EFAULT;
	return -1;
    }
    return stream->fd;
}
