/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function setbuf().
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	void setbuf (

/*  SYNOPSIS */
	FILE * restrict stream,
	char * restrict buf)

/*  FUNCTION
        Sets a buffer associated with a stream.

    INPUTS
        stream: stream to set a buffer for.
        buf: if it points to an array of at least size BUFSIZ it will be used
             as a buffer with mode _IOFBF. If it is NULL mode will be set to
             _IONBF

    RESULT
        -

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        setvbuf()

    INTERNALS

******************************************************************************/
{
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
} /* setbuf */
