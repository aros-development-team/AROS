/*
    Copyright 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function perror()
    Lang: english
*/

#include "errno.h"
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

    HISTORY
	27.04.2001 digulla created

******************************************************************************/
{
    if (string)
    	printf("%s: ", string);

#warning TODO: complete perror()
    printf("Unknown error %d\n", errno);
} /* fopen */

