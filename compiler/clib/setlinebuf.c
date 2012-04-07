/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    BSD legacy function setlinebuf().
    Function is deprecated and not part of POSIX standard.
*/

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

******************************************************************************/
{
    setvbuf(stream, (char *)NULL, _IOLBF, 0);
} /* setlinebuf */

