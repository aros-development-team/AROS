/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strrchr().
*/

#include <aros/macros.h>

/*****************************************************************************

    NAME */
#include <string.h>

	char * strrchr (

/*  SYNOPSIS */
	const char * str,
	int	     c)

/*  FUNCTION
	Searches for the last character c in a string.

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

	// This returns a pointer to the second l in buffer.
	strrchr (buffer, 'l');

	// This returns NULL
	strrchr (buffer, 'x');

    BUGS

    SEE ALSO
	strrchr()

    INTERNALS
	It might seem that the algorithm below is slower than one which
	first finds the end and then walks backwards but that would mean
	to process some characters twice - if the string doesn't contain
	c, it would mean to process every character twice.

******************************************************************************/
{
    char * p = NULL;

    while (*str)
    {
        /* those casts are needed to compare chars > 127 */
	if ((unsigned char)*str == (unsigned char)c)
	    p = (char *)str;

	str ++;
    }

    return p;
} /* strrchr */
