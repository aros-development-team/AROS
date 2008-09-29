/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    C function vsscanf().
*/
/* Original source from libnix */
#include <stdio.h>

static int _vsscanf_get (char ** str)
{
    if (!**str)
	return EOF;

    return *(*str)++;
}

static int _vsscanf_unget (int c, char ** str)
{
    (*str)--;

    return c;
}

/*****************************************************************************

    NAME */
	#include <stdio.h>
#include <stdarg.h>

	int vsscanf (

/*  SYNOPSIS */
	const char *str,
	const char *format,
	va_list     args)

/*  FUNCTION
	Scan a string and convert it into the arguments as specified
	by format.

    INPUTS
	str - Scan this string
	format - A scanf() format string.
	args - A list of arguments for the results

    RESULT
	The number of arguments converted.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	scanf(), sscanf(), fscanf(), vscanf(), vfscanf()

    INTERNALS

******************************************************************************/
{
    int rc;

    rc = __vcscan (&str,
	    (void *)_vsscanf_get,
	    (void *)_vsscanf_unget,
	    format,
	    args
    );

    return rc;
} /* vsscanf */
