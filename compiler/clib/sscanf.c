/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sscanf().
*/

#include <stdarg.h>


/*****************************************************************************

    NAME */
#include <stdio.h>

	int sscanf (

/*  SYNOPSIS */
	const char  *str,
	const char  *format,
	...)

/*  FUNCTION
	Scan the specified string and convert it into the arguments as
	specified by format.

    INPUTS
	str     - The routine examines this string.
	format - Format string. See scanf() for a description
	...    - Arguments for the result

    RESULT
	The number of converted parameters.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fscanf(), vscanf(), vfscanf(), vsscanf()

    INTERNALS

******************************************************************************/
{
    int     retval;
    va_list args;

    va_start(args, format);
    retval = vsscanf(str, format, args);
    va_end(args);
    
    return retval;
}  
