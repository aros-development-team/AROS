/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function bcmp()
    Lang: English
*/

/*****************************************************************************

    NAME */
#include <string.h>

	int bcmp (

/*  SYNOPSIS */
	const void * s1,
	const void * s2,
	int	     n)

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

    HISTORY

******************************************************************************/
{
    return memcmp(s1, s2, (size_t)n);
} /* bcmp */

