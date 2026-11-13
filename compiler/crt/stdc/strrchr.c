/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    C99 function strrchr().
*/

#include <aros/macros.h>

/*****************************************************************************

    NAME */
#include <string.h>

        char * strrchr (

/*  SYNOPSIS */
        const char * str,
        int          c)

/*  FUNCTION
        Searches for the last character c in a string.

    INPUTS
        str - Search this string
        c - Look for this character

    RESULT
        A pointer to the first occurrence of c in str or NULL if c is not
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

    /* The terminating NUL is part of the string, so it is included in the
       search (this lets strrchr(s, '\0') find the end of the string, as
       required by C99 7.21.5.5). */
    do
    {
        /* those casts are needed to compare chars > 127 */
        if ((unsigned char)*str == (unsigned char)c)
            p = (char *)str;
    }
    while (*str++);

    return p;
} /* strrchr */
