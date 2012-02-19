/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function strtok().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	char * strtok (

/*  SYNOPSIS */
	char	   * str,
	const char * sep)

/*  FUNCTION
	Separates a string by the characters in sep.

    INPUTS
	str - The string to check or NULL if the next word in
		the last string is to be searched.
	sep - Characters which separate "words" in str.

    RESULT
	The first word in str or the next one if str is NULL.

    NOTES
	The function changes str !

    EXAMPLE
	char buffer[64];

	strcpy (buffer, "Hello, this is a test.");

	// Init. Returns "Hello"
	strtok (str, " \t,.");

	// Next word. Returns "this"
	strtok (NULL, " \t,.");

	// Next word. Returns "is"
	strtok (NULL, " \t");

	// Next word. Returns "a"
	strtok (NULL, " \t");

	// Next word. Returns "test."
	strtok (NULL, " \t");

	// Next word. Returns NULL.
	strtok (NULL, " \t");

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    static char * t;

    if (str != NULL)
	t = str;
    else
	str = t;

    str += strspn (str, sep);

    if (*str == '\0')
	return NULL;

    t = str;

    t += strcspn (str, sep);

    if (*t != '\0')
	*t ++ = '\0';

    return str;
} /* strtok */
