/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function snprintf().
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	int snprintf (

/*  SYNOPSIS */
	char	   * str,
	size_t	     n,
	const char * format,
	...)

/*  FUNCTION
	Formats a list of arguments and writes them into the string str.

    INPUTS
	str - The formatted string is written into this variable. You
		must make sure that it is large enough to contain the
		result.
	n - At most n characters are written into the string. This
		includes the final 0.
	format - Format string as described above
	... - Arguments for the format string

    RESULT
	The number of characters written into the string. If this is
	-1, then there was not enough room. The 0 byte at the end is not
	included.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fprintf(), vprintf(), vfprintf(), snprintf(), vsprintf(),
	vsnprintf()

    INTERNALS

******************************************************************************/
{
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vsnprintf (str, n, format, args);

    va_end (args);

    return retval;
} /* snprintf */

#ifdef TEST
#include <stdio.h>

int main (int argc, char ** argv)
{
    char buffer[11];
    int  rc;

    printf ("snprintf test\n");

    rc = snprintf (buffer, sizeof (buffer), "%10d", 5);

    if (rc < sizeof (buffer))
	printf ("rc=%d, buffer=\"%s\"\n", rc, buffer);
    else
	printf ("rc=%d\n", rc);

    rc = snprintf (buffer, sizeof (buffer), "%11d", 5);

    if (rc < sizeof (buffer))
	printf ("rc=%d, buffer=\"%s\"\n", rc, buffer);
    else
	printf ("rc=%d\n", rc);

    return 0;
} /* main */

#endif /* TEST */
