/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function strspn()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <string.h>

	size_t strspn (

/*  SYNOPSIS */
	const char * str,
	const char * accept)

/*  FUNCTION
	Calculates the length of the initial segment of str which consists
	entirely of characters in accept.

    INPUTS
	str - The string to check.
	accept - Characters which have to be in str.

    RESULT
	Length of the initial segment of str which contains only
	characters from accept.

    NOTES

    EXAMPLE
	char buffer[64];

	strcpy (buffer, "Hello ");

	// Returns 5
	strspn (buffer, "Helo");

	// Returns 0
	strspn (buffer, "xyz");

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	11.12.1996 digulla created

******************************************************************************/
{
    size_t n = 0;

    while (*str && strchr (accept, *str))
    {
	str ++;
	n ++;
    }

    return n;
} /* strspn */
