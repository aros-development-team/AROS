/*
    Copyright (C) 2008, The AROS Development Team. All rights reserved.
*/


#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>

/* Parse one decimal field using the caller's s, val and matched.
 * Returns from strptime on EOF. A zero-conversion result is not rejected;
 * val may then be stale or uninitialized. The %u destination is incorrectly
 * declared int.
 */
#define MATCHUINT(dest, value)                   \
    if(sscanf(s, "%u%n", &val, &matched) != EOF) \
        dest = value;                            \
    else                                         \
        return NULL;                             \

/****************************************************************************
    NAME
        char * strptime(

    SYNOPSIS
        const char *s, const char *format, struct tm *tm)

    FUNCTION
        Parse selected numeric date and time fields from s into
        existing struct tm storage. Fields are updated as they
        are encountered; the structure is not initialized.

    INPUTS
        s - Readable, null-terminated input string.
        format - Readable, null-terminated format string. Supported
                 conversions are %d, %e, %H, %m, %M, %S, %w, %y,
                 %Y and %%. Other conversions are unsupported.
        tm - Writable result structure, separate from both strings.
             Initialize fields that the format does not supply.

    RESULT
        A pointer into s after the parsed portion, or NULL on a
        reported failure. Trailing input need not be consumed.
        If any argument is NULL, returns NULL with errno set to
        EINVAL before modifying the result.

        On later failure, earlier field assignments remain. Other
        failures do not have a defined errno value. Because of the
        parser defects below, a non-NULL result does not establish
        that the input matched or represents a valid date.

    NOTES
        Include <time.h>. %d and %e assign tm_mday; %H assigns
        tm_hour; %m assigns the parsed month minus one to tm_mon;
        %M assigns tm_min; %S assigns tm_sec; and %w assigns
        tm_wday. %y assigns tm_year directly, while %Y subtracts
        1900. No century window is applied to %y.

        Format whitespace matches zero or more input whitespace
        characters. Other ordinary characters must match exactly.
        Numeric fields use decimal scanning without field-width
        limits. Unspecified fields, including tm_yday and
        tm_isdst, remain unchanged; no derived fields are computed.

    BUGS
        Numeric parsing accepts a failed zero-conversion scan as
        success and can assign an uninitialized or previous value.
        It also passes int storage to an unsigned scanf conversion,
        violating that conversion's argument-type contract.

        A failed %% match moves the input pointer backwards
        instead of reporting failure, potentially outside the
        input object. A trailing % in format is silently accepted.

        Field ranges, arithmetic overflow and calendar consistency
        are not checked. Names, locale-specific formats, time
        zones and all unlisted conversions are unsupported.

    SEE ALSO
        strftime(), sscanf()

*****************************************************************************/
char *strptime(const char *s, const char *format, struct tm *tm)
{
    int matched;
    int val;
    
    if (format == NULL || s == NULL || tm == NULL)
    {
        errno = EINVAL;
        return NULL;
    }
    
    while (*format)
    {
        if (*format == '%')
        {
            matched = 0;
            
            switch(*++format)
            {
            case '%':
                matched = (*s == '%' ? 1 : EOF);
                break;
            case 'd':
            case 'e':
                MATCHUINT(tm->tm_mday, val);
                break;
            case 'H':
                MATCHUINT(tm->tm_hour, val);
                break;
            case 'm':
                MATCHUINT(tm->tm_mon, val - 1);
                break;
            case 'M':
                MATCHUINT(tm->tm_min, val);
                break;
            case 'S':
                MATCHUINT(tm->tm_sec, val);
                break;
            case 'w':
                MATCHUINT(tm->tm_wday, val);
                break;
            case 'y':
                MATCHUINT(tm->tm_year, val);
                break;
            case 'Y':
                MATCHUINT(tm->tm_year, val - 1900);
                break;
            case '\0':
                format--;
                break;
            default:
                /* FIXME: Implement remaining conversions */
                return 0;
            }
            
            s += matched;
        }
        else
        {
            /* whitespace matches zero or more whitespace characters */
            if(isspace(*format))
            {
                while(isspace(*s))
                    s++;
            }
            else
            {
                /* compare characters directly */
                if(*s != *format)
                    return NULL;
                s++;
            }
        }
        
        format++;
    }

    return (char *)s;
}
