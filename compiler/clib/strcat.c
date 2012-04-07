/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strcat().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	char * strcat (

/*  SYNOPSIS */
	char	   * dest,
	const char * src)

/*  FUNCTION
	Concatenates two strings.

    INPUTS
	dest - src is appended to this string. Make sure that there
		is enough room for src.
	src - This string is appended to dest

    RESULT
	dest.

    NOTES
	The routine makes no checks if dest is large enough.

    EXAMPLE
	char buffer[64];

	strcpy (buffer, "Hello ");
	strcat (buffer, "World.");

	// Buffer now contains "Hello World."

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char * d = dest;

    while (*dest)
	dest ++;

    while ((*dest ++ = *src ++));

    return d;
} /* strcat */
