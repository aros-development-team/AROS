/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strtof().
*/

#ifndef AROS_NOFPU

#include <ctype.h>
#include <limits.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>
#include <math.h>

        float strtof (

/*  SYNOPSIS */
        const char * str,
        char      ** endptr)

/*  FUNCTION
        Convert a floating-point number from an ASCII decimal
        representation into a double-precision format.

    INPUTS
        str - The string which should be converted. Leading
                whitespace are ignored.
        endptr - If this is non-NULL, then the address of the first
                character after the number in the string is stored
                here.

    RESULT
        The float value of the string. The first character after the number
        is returned in *endptr, if endptr is non-NULL. If no digits can
        be converted, *endptr contains str (if non-NULL) and 0 is
        returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        strtod(), strtold()

    INTERNALS

******************************************************************************/
{
    return strtod(str, endptr);
}

#endif /* AROS_NOFPU */
