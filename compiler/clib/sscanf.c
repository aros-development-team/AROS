/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function sscanf()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	int sscanf (

/*  SYNOPSIS */
	char	   * str,
	const char * format,
	...)

/*  FUNCTION
	Scan the specified string and convert it into the arguments as
	specified by format.

    INPUTS
	str - The routine examines this string.
	format - Format string. See scanf() for a description
	... - Arguments for the result

    RESULT
	The number of converted parameters.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fscanf(), vscanf(), vfscanf(), snscanf(), vsscanf(),
	vnsscanf()

    INTERNALS

    HISTORY
	06.12.1996 digulla created

******************************************************************************/
{
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vsscanf (str, format, args);

    va_end (args);

    return retval;
} /* sscanf */

