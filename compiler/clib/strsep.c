/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    BSD function strsep().
*/

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <string.h>

	char * strsep (

/*  SYNOPSIS */
	char	   ** strptr,
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
        char **bufptr

	strcpy (buffer, "Hello, this is a test.");
        *bufptr = buffer

        // First word. Returns "Hello"
	strtok (bufptr, " \t,.");

	// Next word. Returns "this"
	strtok (bufptr, " \t,.");

	// Next word. Returns "is"
	strtok (bufptr, " \t");

	// Next word. Returns "a"
	strtok (bufptr, " \t");

	// Next word. Returns "test."
	strtok (bufptr, " \t");

	// Next word. Returns NULL.
	strtok (bufptr, " \t");

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char * retval;
    
    if (*strptr == NULL)
	return NULL;

    *strptr += strspn (*strptr, sep);

    if (**strptr == '\0')
    {
	*strptr = NULL;
	return NULL;
    }
    else
	retval = *strptr;
    
    *strptr += strcspn (*strptr, sep);

    if (**strptr != '\0')
	*(*strptr) ++ = '\0';
    else
	*strptr = NULL;

    return retval;
} /* strsep */
