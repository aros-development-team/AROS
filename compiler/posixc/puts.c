/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function puts().
*/

#include <libraries/posixc.h>

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
    struct PosixCBase *PosixCBase = __aros_getbase_PosixCBase();

    if
    (
        fputs (str, PosixCBase->_stdout)  == EOF ||
        fputs ("\n", PosixCBase->_stdout) == EOF ||
        fflush (PosixCBase->_stdout)      == EOF
    )
    {
	return EOF;
    }

    return 1;
} /* puts */

