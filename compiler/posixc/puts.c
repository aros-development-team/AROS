/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    C99 function puts().
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	int puts (

/*  SYNOPSIS */
	const char * str)

/*  FUNCTION
	Print a string to stdout. A newline ('\n') is emmitted after the
	string.

    INPUTS
	str - Print this string

    RESULT
	> 0 on success and EOF on error. On error, the reason is put in
	errno.

    NOTES

    EXAMPLE
	#include <errno.h>

	if (puts ("Hello World.") != EOF)
	    fprintf (stderr, "Success");
	else
	    fprintf (stderr, "Failure: errno=%d", errno);

    BUGS

    SEE ALSO
	fputs(), printf(), fprintf(), putc(), fputc()

    INTERNALS

******************************************************************************/
 {
    if
    (
        fputs (str, stdout)  == EOF ||
        fputs ("\n", stdout) == EOF ||
        fflush (stdout)      == EOF
    )
    {
	return EOF;
    }

    return 1;
} /* puts */

