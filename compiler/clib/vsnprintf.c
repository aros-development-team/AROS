/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: C function vsnprintf()
    Lang: english
*/
/* Original source from libnix */

#define AROS_ALMOST_COMPATIBLE
#include <stdio.h>

struct data
{
    char * str;
    size_t n;
};

static int _vsnprintf_uc (int c, struct data * data)
{
    if (data->n)
    {
	*(data->str) ++ = c;
	data->n --;

	return 1;
    }

    return EOF;
}

/*****************************************************************************

    NAME */
	#include <stdio.h>
#include <stdarg.h>

	int vsnprintf (

/*  SYNOPSIS */
	char	   * str,
	size_t	     n,
	const char * format,
	va_list      args)

/*  FUNCTION
	Format a list of arguments and put them into the string str.
	The function makes sure that no more than n characters (including
	the terminal 0 byte) are written into str.

    INPUTS
	str - The formatted result is stored here
	n - The size of str
	format - A printf() format string.
	args - A list of arguments for the format string.

    RESULT
	The number of characters written or -1 if the string was too small.
	In this case, the string is not 0-terminated.

    NOTES
	No check is beeing made that str is large enough to contain
	the result.

    EXAMPLE

    BUGS

    SEE ALSO
	printf(), sprintf(), fprintf(), vprintf(), vfprintf(), snprintf(),
	vsnprintf()

    INTERNALS

    HISTORY
	11.12.1996 digulla created

******************************************************************************/
{
    int rc;
    struct data data;

    data.n = n;
    data.str = str;

    rc = __vcformat (&data, (void *)_vsnprintf_uc, format, args);

    if (data.n)
    {
	*(data.str) = 0;

	return (n - data.n);
    }

    return -1;
} /* vsnprintf */
