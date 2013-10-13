/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
  
    POSIX.1-2001 function strtok_r().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	char * strtok_r (

/*  SYNOPSIS */
	char	   * str,
	const char * sep,
	char **saveptr)

/*  FUNCTION
	Separates a string by the characters in sep.

    INPUTS
	str - The string to check or NULL if the next word in
		the last string is to be searched.
	sep - Characters which separate "words" in str.
	saveptr - internal context for next scan

    RESULT
	The first word in str or the next one if str is NULL.

    NOTES
	The function changes str !

    EXAMPLE
	char buffer[64];
	char *ptr;

	strcpy (buffer, "Hello, this is a test.");

	// Init. Returns "Hello"
	strtok_r (str, " \t,.", &ptr);

	// Next word. Returns "this"
	strtok_r (NULL, " \t,.", &ptr);

	// Next word. Returns "is"
	strtok_r (NULL, " \t", &ptr);

	// Next word. Returns "a"
	strtok_r (NULL, " \t", &ptr);

	// Next word. Returns "test."
	strtok_r (NULL, " \t", &ptr);

	// Next word. Returns NULL.
	strtok_r (NULL, " \t", &ptr);

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char * t = *saveptr;

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

    *saveptr = t;
    return str;
} /* strtok_r */
