/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function bcmp().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	int bcmp (

/*  SYNOPSIS */
	const void * s1,
	const void * s2,
	size_t	     n)

/*  FUNCTION
	Compare the first n bytes of s1 and s2.

    INPUTS
	s1 - The first byte array to be compared
	s2 - The second byte array to be compared
	n  - How many bytes to compare

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    return memcmp(s1, s2, n);
} /* bcmp */

