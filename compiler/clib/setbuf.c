/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function setbuf()
    Lang: english
*/

#define setbuf setbuf

/*****************************************************************************

    NAME */
#include <stdio.h>

	int setbuf (

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

    HISTORY

******************************************************************************/
{

    return setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);

} /* setbuf */

