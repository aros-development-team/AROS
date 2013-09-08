/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function strndup().
*/

#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <string.h>

	char * strndup (

/*  SYNOPSIS */
	const char *s, size_t n)

/*  FUNCTION
	Create a copy of a string. The copy can be freed with free() or will
	be freed when then program ends. The copy will be at most n character
	long, excluding the trailing \000

    INPUTS
	s - String to duplicate
	n - Maximum length

    RESULT
	A copy of the string which can be freed with free().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char * copy;
    int len = strlen(s);

    if (len > n)
        len = n;

    if ((copy = malloc (len+1)))
    {
        memcpy(copy, s, len);
        copy[len] = 0;
    }

    return copy;
} /* strndup */

