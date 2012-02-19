/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function snprintf().
*/

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        int snprintf (

/*  SYNOPSIS */
        char       * str,
        size_t       n,
        const char * format,
        ...)

/*  FUNCTION
        C99 says:The snprintf function is equivalent to fprintf, except that the output is
        written into an array (specified by argument s) rather than to a stream. If
        n is zero, nothing is written, and s may be a null pointer. Otherwise,
        output characters beyond the n-1st are discarded rather than being written
        to the array, and a null character is written at the end of the characters
        actually written into the array. If copying takes place between objects
        that overlap, the behavior is undefined.

    INPUTS
        str - The formatted string is written into this variable. You
              must make sure that it is large enough to contain the
              result.
        n -   At most n characters are written into the string. This
              includes the final 0.
        format - Format string as described above
        ... - Arguments for the format string

    RESULT
        The snprintf function returns the number of characters that would have been
        written had n been sufficiently large, not counting the terminating null
        character, or a negative value if an encoding error occurred. Thus, the
        null-terminated output has been completely written if and only if the
        returned value is nonnegative and less than n.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        fprintf(), vprintf(), vfprintf(), snprintf(), vsprintf(),
        vsnprintf()

    INTERNALS

******************************************************************************/
{
    int     retval;
    va_list args;

    D(bug("[snprintf] start; str %p('%s'), format: %p('%s')\n",
          str, str, format, format
    ));

    va_start (args, format);

    retval = vsnprintf (str, n, format, args);

    va_end (args);

    D(bug("[snprintf] end; str %p('%s'), format: %p('%s')\n",
          str, str, format, format
    ));

    return retval;
} /* snprintf */

#ifdef TEST
#include <stdio.h>

int main (int argc, char ** argv)
{
    char buffer[11];
    int  rc;

    printf ("snprintf test\n");

    rc = snprintf (buffer, sizeof (buffer), "%10d", 5);

    if (rc < sizeof (buffer))
        printf ("rc=%d, buffer=\"%s\"\n", rc, buffer);
    else
        printf ("rc=%d\n", rc);

    rc = snprintf (buffer, sizeof (buffer), "%11d", 5);

    if (rc < sizeof (buffer))
        printf ("rc=%d, buffer=\"%s\"\n", rc, buffer);
    else
        printf ("rc=%d\n", rc);

    return 0;
} /* main */

#endif /* TEST */
