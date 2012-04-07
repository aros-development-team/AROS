/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2001 function bcmp().
    Function is deprecated and removed from POSIX.1-2008
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

