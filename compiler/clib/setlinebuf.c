/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function setlinebuf()
    Lang: english
*/


/*****************************************************************************

    NAME */
#include <stdio.h>

	int setlinebuf (

/*  SYNOPSIS */
	FILE *stream)

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

    return setvbuf(stream, (char *)NULL, _IOLBF, 0);

} /* setlinebuf */

