/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.

    BSD function strsep().
*/

/*****************************************************************************

    NAME */
#include <string.h>

        char * strsep (

/*  SYNOPSIS */
        char       ** strptr,
        const char * sep)

/*  FUNCTION
        Separates a string by the characters in sep.

        Unlike strtok(), strsep() does not merge adjacent delimiters: an
        empty field (two delimiters in a row, or a leading delimiter) yields
        an empty string. This makes it suitable for parsing records whose
        fields may be empty.

    INPUTS
        strptr - Address of the string to scan. Updated to point just after
                 the delimiter that terminated the returned token, or set to
                 NULL once the end of the string has been reached.
        sep - Characters which separate "words" in the string.

    RESULT
        A pointer to the (possibly empty) token, or NULL when *strptr is
        already NULL (no more tokens).

    NOTES
        The function changes the scanned string (it writes a NUL over the
        delimiter that terminates each token).

    EXAMPLE
        char buffer[64];
        char *bufptr, *word;

        strcpy (buffer, "a,bb,,ccc");
        bufptr = buffer;

        word = strsep (&bufptr, ",");   // Returns "a"
        word = strsep (&bufptr, ",");   // Returns "bb"
        word = strsep (&bufptr, ",");   // Returns ""   (empty field!)
        word = strsep (&bufptr, ",");   // Returns "ccc"
        word = strsep (&bufptr, ",");   // Returns NULL

    BUGS

    SEE ALSO
        strtok()

    INTERNALS

******************************************************************************/
{
    char *s = *strptr;
    char *end;

    if (s == NULL)
        return NULL;

    /* Find the end of the token: the first delimiter, or the terminating
       NUL. Leading delimiters are NOT skipped, so an empty field is
       returned as an empty string (this is what distinguishes strsep()
       from strtok()). */
    end = s + strcspn(s, sep);

    if (*end != '\0')
        *end++ = '\0';      /* terminate the token; resume after delimiter */
    else
        end = NULL;         /* this was the final token */

    *strptr = end;

    return s;
} /* strsep */
