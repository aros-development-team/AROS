/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function memcmp().
*/

#include <exec/types.h>

/*****************************************************************************

    NAME */
#include <string.h>

	int memcmp (

/*  SYNOPSIS */
	const void * s1,
	const void * s2,
	size_t	     n)

/*  FUNCTION
	Calculate s1 - s2 for the n bytes after s1 and s2 and stop when
	the difference is not 0.

    INPUTS
	s1, s2 - Begin of the memory areas to compare
	n - The number of bytes to compare

    RESULT
	The difference of the memory areas. The difference is 0, if both
	are equal, < 0 if s1 < s2 and > 0 if s1 > s2. Note that it may be
	greater then 1 or less than -1.

    NOTES
	This function is not part of a library and may thus be called
	any time.

    EXAMPLE

    BUGS

    SEE ALSO
	strcmp(), strncmp(), strcasecmp(), strncasecmp()

    INTERNALS

******************************************************************************/
{
    const UBYTE * str1,
		* str2;
    int 	  diff = 0; /* In case we are comparing n == 0 */


    str1 = s1;
    str2 = s2;

    while (n && !(diff = *str1 - *str2))
    {
	str1 ++;
	str2 ++;
	n --;
    }

    /* Now return the difference. */
    return diff;
} /* memcmp */

