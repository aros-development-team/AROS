/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    C function vsnprintf().
*/
/* Original source from libnix */

#include <stdarg.h>
#include <stddef.h>

struct data
{
    char * str;
    size_t n;
};

static int _vsnprintf_uc (int c, struct data * data)
{
    if (data->n)
    {
        *(data->str) ++ = c;
        data->n --;
    }

    return 1;
}

/*****************************************************************************

    NAME */
#include <stdio.h>

        int vsnprintf (

/*  SYNOPSIS */
        char        * str,
        size_t      n,
        const char * format,
        va_list      args)

/*  FUNCTION
        Format a list of arguments and put them into the string str.
        The function makes sure that no more than n characters (including
        the terminal 0 byte) are written into str.
        If n is zero, nothing is written, and s may be a null pointer. Otherwise,
        output characters beyond the n-1st are discarded rather than being written
        to the array, and a null character is written at the end of the characters
        actually written into the array. If copying takes place between objects
        that overlap, the behavior is undefined.
    
    INPUTS
        str - The formatted result is stored here
        n - The size of str
        format - A printf() format string.
        args - A list of arguments for the format string.

    RESULT
        Function returns the number of characters that would have been
        written had n been sufficiently large, not counting the terminating null
        character, or a negative value if an encoding error occurred. Thus, the
        null-terminated output has been completely written if and only if the
        returned value is nonnegative and less than n.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        printf(), sprintf(), fprintf(), vprintf(), vfprintf(), snprintf(),
        vsnprintf()

    INTERNALS

******************************************************************************/
{
    int rc;
    struct data data;

    if (str == NULL && n != 0) n = 0;

    data.n = n;
    data.str = str;

    rc = __vcformat (&data, (void *)_vsnprintf_uc, format, args);

    if (data.n)
    {
        *(data.str) = 0;
    }
    else if ( n != 0 )
    {
        str[n-1] = 0;
    }

    return rc;
} /* vsnprintf */

