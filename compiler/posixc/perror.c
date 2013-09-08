/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function perror().
*/

#include <errno.h>
#include <string.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	void perror (

/*  SYNOPSIS */
	const char *string
	)

/*  FUNCTION
	looks up the language-dependent error message string affiliated with an error
	number and writes it, followed by a newline, to the standard error stream.

    INPUTS
	string - the string to prepend the error message. If NULL only the error
	         message will be printed, otherwise the error message will be
		 separated from string by a colon.
    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    if (string)
    {
	fputs(string, stderr);
	fputs(": ", stderr);
    }

    fputs(__posixc_strerror(errno), stderr);
    fputs("\n", stderr);

} /* perror */

