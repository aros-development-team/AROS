/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    C function vsnprintf().
*/
/* Original source from libnix */

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
    }

    return 1;
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
	The number of characters written into the string. The 0 byte at the
	end is not included. If this is greater than or equal to n then
	there was not enough room to write all characters. In this case the
	output string is not null-terminated, and the return value is the
	number of characters which would have been written if enough space had
	been available.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	printf(), sprintf(), fprintf(), vprintf(), vfprintf(), snprintf(),
	vsnprintf()

    INTERNALS

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
    }

    return rc;
} /* vsnprintf */
