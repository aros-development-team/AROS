/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function setlinebuf()
    Lang: english
*/

#define setlinebuf setlinebuf

/*****************************************************************************

    NAME */
#include <stdio.h>

	void setlinebuf (

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

    setvbuf(stream, (char *)NULL, _IOLBF, 0);

} /* setlinebuf */

