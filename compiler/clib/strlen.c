/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function strlen()
    Lang: english
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

    HISTORY
	29.07.1996 digulla created

******************************************************************************/
{
    const char * start = ptr;

    while (*ptr) ptr ++;

    return (((long)ptr) - ((long)start));
} /* strlen */

