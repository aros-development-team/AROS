/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strpbrk().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	char * strpbrk (

/*  SYNOPSIS */
	const char * str,
	const char * accept)

/*  FUNCTION
	Locate the first occurrence of any character in accept in str.

    INPUTS
	str - Search this string
	accept - Look for these characters

    RESULT
	A pointer to the first occurence of any character in accept in str
	or NULL if no character of accept is not found in str.

    NOTES

    EXAMPLE
	char buffer[64];

	strcpy (buffer, "Hello ");

	// This returns a pointer to the first l in buffer.
	strpbrk (buffer, "lo");

	// This returns NULL
	strpbrk (buffer, "xyz");

    BUGS

    SEE ALSO
	strchr(), strrchr()

    INTERNALS

******************************************************************************/
{
    while (*str)
    {
	if (strchr (accept, *str))
	    return ((char *)str);

	str ++;
    }

    return(0);
} /* strpbrk */
