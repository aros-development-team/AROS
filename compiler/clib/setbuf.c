/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function setbuf().
*/

#define setbuf setbuf

/*****************************************************************************

    NAME */
#include <stdio.h>

	void setbuf (

/*  SYNOPSIS */
	FILE *stream,
	char *buf)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
	This is a simpler alias for setvbuf() according to manpage.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
} /* setbuf */

