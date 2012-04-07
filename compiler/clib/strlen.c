/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strlen().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	size_t strlen (

/*  SYNOPSIS */
	const char * ptr)

/*  FUNCTION
	Calculate the length of a string (without the terminating 0 byte).

    INPUTS
	ptr - The string to get its length for

    RESULT
	The length of the string.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    const char * start = ptr;

    while (*ptr) ptr ++;

    return (((long)ptr) - ((long)start));
} /* strlen */

