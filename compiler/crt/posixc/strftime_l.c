/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <time.h>
#include <string.h>
#include <stdio.h>

/*****************************************************************************

    NAME */

        size_t strftime_l(

/*  SYNOPSIS */
                  char *restrict buf, size_t maxsize,
                  const char *restrict format,
                  const struct tm *restrict tm,
                  locale_t locale)

/*  FUNCTION

        Converts the time information in the struct tm pointed to by 'tm'
        into a formatted string according to the 'format' specification and
        the specified 'locale'.

        The resulting string is placed in the buffer pointed to by 'buf', which
        can contain up to 'maxsize' characters, including the terminating null.

    INPUTS

        buf         - Pointer to the destination character buffer.
        maxsize     - Maximum number of characters to store in the destination,
                      including the null terminator.
        format      - Format control string containing ordinary characters and
                      conversion specifiers, similar to ANSI strftime().
        tm          - Pointer to a struct tm containing the time values to format.
        locale      - The locale to use for localized string lookups. If NULL,
                      the current process or system default locale is used.

    RESULT
        The number of characters written to the destination buffer, excluding the
        terminating null character.

        Returns 0 if an error occurs (e.g., invalid arguments or formatting failure).

    NOTES
        Supports a subset of ANSI C strftime() specifiers:
        %a %A - abbreviated and full weekday names
        %b %B - abbreviated and full month names
        %p    - AM/PM indicator
        %d %m %y %Y %H %I %M %S - numeric date/time fields
        %n %t %% - special characters

        Composite formats (%c, %x, %X, etc.) and locale-specific date/time formats
        are not yet implemented.

    EXAMPLE

        struct tm now = { .tm_year = 124, .tm_mon = 6, .tm_mday = 7, .tm_wday = 0,
                          .tm_hour = 15, .tm_min = 30, .tm_sec = 45 };
        char buf[64];

        strftime_l(buf, sizeof(buf), "%A, %B %d, %Y %H:%M:%S %p", &now, NULL);

        // Example output: "Sunday, July 07, 2024 15:30:45 PM"

    BUGS
        - Does not support all strftime() specifiers.
        - Locale argument may be ignored if only a global locale is supported.

    SEE ALSO

        strftime()

    INTERNALS


******************************************************************************/
{
    return strftime(buf, maxsize, format, tm);
}

