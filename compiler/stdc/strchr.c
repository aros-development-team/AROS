/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strchr().
*/

#include <aros/macros.h>
#include <stdio.h>

/*****************************************************************************

    NAME */
#include <string.h>

	char * strchr (

/*  SYNOPSIS */
	const char * str,
	int	     c)

/*  FUNCTION
	Searches for a character in a string.

    INPUTS
	str - Search this string
	c - Look for this character

    RESULT
	A pointer to the first occurence of c in str or NULL if c is not
	found in str.

    NOTES

    EXAMPLE
	char buffer[64];

	strcpy (buffer, "Hello ");

	// This returns a pointer to the first l in buffer.
	strchr (buffer, 'l');

	// This returns NULL
	strchr (buffer, 'x');

    BUGS

    SEE ALSO
	strrchr()

    INTERNALS

******************************************************************************/
{
    do
    {
        /* those casts are needed to compare chars > 127 */
	if ((unsigned char)*str == (unsigned char)c)
	    return ((char *)str);
    } while (*(str++));

    return NULL;
} /* strchr */
