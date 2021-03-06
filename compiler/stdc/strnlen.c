/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.

    C99 function strnlen().
*/

/*****************************************************************************

    NAME */
#include <string.h>

        size_t strnlen (

/*  SYNOPSIS */
        const char * ptr, size_t n)

/*  FUNCTION
        Calculate the length of a string (without the terminating 0 byte).

    INPUTS
        ptr - The string to get its length for
        n - The max length

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

    while (*ptr && n)
    {
        ptr ++;
        n--;
    }

    return (((long)ptr) - ((long)start));
} /* strnlen */

