/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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
        This function is not part of POSIX and programmers are advised 
        to use setvbuf() function directly.
        Legacy functions may be removed in the future.

    EXAMPLE

    BUGS

    SEE ALSO
        setvbuf()

    INTERNALS
        This function is part of the static link lib and not in
        posixc.library.

******************************************************************************/
{
    setvbuf(stream, (char *)NULL, _IOLBF, 0);
} /* setlinebuf */

