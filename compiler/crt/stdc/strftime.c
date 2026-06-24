/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <libraries/localestd.h>
#include <libraries/locale.h>
#include <proto/locale.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "__stdc_intbase.h"
#include "__optionallibs.h"

#define ADDS(st)  do { tmp = strlen(st); if (size + tmp < maxsize) { strcpy(s + size, st); size += tmp; } } while (0)
#define ADDN(a,b) do { tmp = __strfnumb_locale(s + size, maxsize - size, (a), (b)); size += tmp; } while (0)
#define STOR(c)   do { if (size < maxsize) s[size++] = (c); } while(0)

// Global default day and month names fallback
extern CONST_STRPTR DefaultAbDay[7];
extern CONST_STRPTR DefaultDay[7];
extern CONST_STRPTR DefaultAbMon[12];
extern CONST_STRPTR DefaultMon[12];
extern CONST_STRPTR DefaultAM;
extern CONST_STRPTR DefaultPM;

/* Helper for writing numbers with optional padding */
static size_t __strfnumb_locale(char *s, size_t maxsize, signed int places, size_t value)
{
    char buffer[12];
    size_t len = 0;
    int width = places < 0 ? -places : places;
    int pos = sizeof(buffer) - 1;

    buffer[pos--] = '\0';

    do {
        buffer[pos--] = '0' + (value % 10);
        value /= 10;
    } while (value > 0 && pos >= 0);

    while (places > 0 && (sizeof(buffer) - 2 - pos) < (unsigned)places && pos >= 0)
        buffer[pos--] = '0';

    while (places < 0 && (sizeof(buffer) - 2 - pos) < (unsigned)width && pos >= 0)
        buffer[pos--] = ' ';

    len = 0;
    while (buffer[++pos] && len < maxsize)
        *s++ = buffer[pos], len++;

    return len;
}

static int __strf_is_leap(int year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

/* ISO 8601 week number (1..53); sets *iso_year to the ISO week-based year.
   The week of a date is the week whose Thursday it shares, and that Thursday's
   year is the week-based year.  tm_yday is 0-based, tm_wday is 0=Sunday. */
static int __strf_iso_week(const struct tm *tm, int *iso_year)
{
    int year     = tm->tm_year + 1900;
    int iso_wday  = (tm->tm_wday + 6) % 7;          /* 0=Mon .. 6=Sun */
    int thursday  = tm->tm_yday - iso_wday + 3;     /* this week's Thursday (yday) */

    if (thursday < 0)
    {
        year--;
        thursday += __strf_is_leap(year) ? 366 : 365;
    }
    else if (thursday >= (__strf_is_leap(year) ? 366 : 365))
    {
        thursday -= __strf_is_leap(year) ? 366 : 365;
        year++;
    }

    *iso_year = year;
    return thursday / 7 + 1;
}

static size_t __strftime_locale(char *s, size_t maxsize, const char *format, const struct tm *timeptr, struct Locale *locale)
{
    size_t size = 0;
    size_t tmp;

    if (!s || !format || !timeptr || maxsize == 0)
        return 0;

    while (*format && size < maxsize - 1)
    {
        if (*format == '%')
        {
            tmp = 0;
            ++format;

            switch (*format)
            {
                case 'a':
                {
                    CONST_STRPTR str = locale ? GetLocaleStr(locale, ABDAY_1 + timeptr->tm_wday) : DefaultAbDay[timeptr->tm_wday];
                    tmp = snprintf(s, maxsize - size, "%s", str);
                    break;
                }

                case 'A':
                {
                    CONST_STRPTR str = locale ? GetLocaleStr(locale, DAY_1 + timeptr->tm_wday) : DefaultDay[timeptr->tm_wday];
                    tmp = snprintf(s, maxsize - size, "%s", str);
                    break;
                }

                case 'b':
                case 'h':
                {
                    CONST_STRPTR str = locale ? GetLocaleStr(locale, ABMON_1 + timeptr->tm_mon) : DefaultAbMon[timeptr->tm_mon];
                    tmp = snprintf(s, maxsize - size, "%s", str);
                    break;
                }

                case 'B':
                {
                    CONST_STRPTR str = locale ? GetLocaleStr(locale, MON_1 + timeptr->tm_mon) : DefaultMon[timeptr->tm_mon];
                    tmp = snprintf(s, maxsize - size, "%s", str);
                    break;
                }

                case 'p':
                {
                    CONST_STRPTR str = locale ? GetLocaleStr(locale, AM_STR + (timeptr->tm_hour >= 12)) :
                                               (timeptr->tm_hour >= 12 ? DefaultPM : DefaultAM);
                    tmp = snprintf(s, maxsize - size, "%s", str);
                    break;
                }

                case 'd':
                    tmp = __strfnumb_locale(s, maxsize - size, 2, timeptr->tm_mday);
                    break;

                case 'm':
                    tmp = __strfnumb_locale(s, maxsize - size, 2, timeptr->tm_mon + 1);
                    break;

                case 'y':
                    tmp = __strfnumb_locale(s, maxsize - size, 2, timeptr->tm_year % 100);
                    break;

                case 'Y':
                    tmp = __strfnumb_locale(s, maxsize - size, 4, timeptr->tm_year + 1900);
                    break;

                case 'H':
                    tmp = __strfnumb_locale(s, maxsize - size, 2, timeptr->tm_hour);
                    break;

                case 'I':
                {
                    int hour = timeptr->tm_hour % 12;
                    if (hour == 0) hour = 12;
                    tmp = __strfnumb_locale(s, maxsize - size, 2, hour);
                    break;
                }

                case 'M':
                    tmp = __strfnumb_locale(s, maxsize - size, 2, timeptr->tm_min);
                    break;

                case 'S':
                    tmp = __strfnumb_locale(s, maxsize - size, 2, timeptr->tm_sec);
                    break;

                case 'n':
                    if (size + 1 < maxsize) { s[0] = '\n'; tmp = 1; }
                    break;

                case 't':
                    if (size + 1 < maxsize) { s[0] = '\t'; tmp = 1; }
                    break;

                case '%':
                    if (size + 1 < maxsize) { s[0] = '%'; tmp = 1; }
                    break;

                case 'C':
                    tmp = __strfnumb_locale(s, maxsize - size, 2,
                              (timeptr->tm_year + 1900) / 100);
                    break;

                case 'e':
                    tmp = __strfnumb_locale(s, maxsize - size, -2, timeptr->tm_mday);
                    break;

                case 'j':
                    tmp = __strfnumb_locale(s, maxsize - size, 3, timeptr->tm_yday + 1);
                    break;

                case 'u':
                    tmp = __strfnumb_locale(s, maxsize - size, 1,
                              timeptr->tm_wday == 0 ? 7 : timeptr->tm_wday);
                    break;

                case 'w':
                    tmp = __strfnumb_locale(s, maxsize - size, 1, timeptr->tm_wday);
                    break;

                case 'k':
                    tmp = __strfnumb_locale(s, maxsize - size, -2, timeptr->tm_hour);
                    break;

                case 'l':
                {
                    int hour = timeptr->tm_hour % 12;
                    if (hour == 0) hour = 12;
                    tmp = __strfnumb_locale(s, maxsize - size, -2, hour);
                    break;
                }

                case 'U':
                    tmp = __strfnumb_locale(s, maxsize - size, 2,
                              (timeptr->tm_yday + 7 - timeptr->tm_wday) / 7);
                    break;

                case 'W':
                    tmp = __strfnumb_locale(s, maxsize - size, 2,
                              (timeptr->tm_yday + 7 - ((timeptr->tm_wday + 6) % 7)) / 7);
                    break;

                case 'V':
                {
                    int iso_year;
                    int week = __strf_iso_week(timeptr, &iso_year);
                    tmp = __strfnumb_locale(s, maxsize - size, 2, week);
                    break;
                }

                case 'G':
                {
                    int iso_year;
                    (void)__strf_iso_week(timeptr, &iso_year);
                    tmp = __strfnumb_locale(s, maxsize - size, 4, iso_year);
                    break;
                }

                case 'g':
                {
                    int iso_year;
                    (void)__strf_iso_week(timeptr, &iso_year);
                    tmp = __strfnumb_locale(s, maxsize - size, 2, iso_year % 100);
                    break;
                }

                case 'P':
                    tmp = snprintf(s, maxsize - size, "%s",
                              timeptr->tm_hour >= 12 ? "pm" : "am");
                    break;

                case 'z':
                    /* AROS keeps broken-down time in UTC with no offset. */
                    tmp = snprintf(s, maxsize - size, "%s", "+0000");
                    break;

                case 'Z':
                    /* No timezone name is available; emit nothing (allowed). */
                    break;

                case 'D':
                    tmp = __strftime_locale(s, maxsize - size, "%m/%d/%y", timeptr, locale);
                    break;

                case 'F':
                    tmp = __strftime_locale(s, maxsize - size, "%Y-%m-%d", timeptr, locale);
                    break;

                case 'R':
                    tmp = __strftime_locale(s, maxsize - size, "%H:%M", timeptr, locale);
                    break;

                case 'T':
                    tmp = __strftime_locale(s, maxsize - size, "%H:%M:%S", timeptr, locale);
                    break;

                case 'r':
                    tmp = __strftime_locale(s, maxsize - size, "%I:%M:%S %p", timeptr, locale);
                    break;

                case 'c':
                    tmp = __strftime_locale(s, maxsize - size, "%a %b %e %H:%M:%S %Y", timeptr, locale);
                    break;

                case 'x':
                    tmp = __strftime_locale(s, maxsize - size, "%m/%d/%y", timeptr, locale);
                    break;

                case 'X':
                    tmp = __strftime_locale(s, maxsize - size, "%H:%M:%S", timeptr, locale);
                    break;

                default:
                    // Unrecognized specifier: skip it
                    break;
            }

            size += tmp;
            s += tmp;
        }
        else
        {
            *s++ = *format;
            size++;
        }

        format++;
    }

    *s = '\0';
    return size;
}

static struct Locale * __crt_get_locale()
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    struct Locale *locale = NULL;
    if (__locale_available(StdCBase))
    {
        locale = OpenLocale(NULL);  // currently uses the default system locale (TODO: Fix)
    }
    return locale;
}

/*****************************************************************************

    NAME */

        size_t strftime(

/*  SYNOPSIS */
            char *s,
            size_t maxsize,
            const char *format,
            const struct tm *timeptr)

/*  FUNCTION

        Converts the time information in the struct tm pointed to by 'timeptr'
        into a formatted string according to the 'format' specification.

        The resulting string is placed in the buffer pointed to by 's', which
        can contain up to 'maxsize' characters, including the terminating null.

    INPUTS

        s           - Pointer to the destination character buffer.
        maxsize     - Maximum number of characters to store in the destination,
                      including the null terminator.
        format      - Format control string containing ordinary characters and
                      conversion specifiers, similar to ANSI strftime().
        timeptr     - Pointer to a struct tm containing the time values to format.

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

        strftime(buf, sizeof(buf), "%A, %B %d, %Y %H:%M:%S %p", &now);

        // Example output: "Sunday, July 07, 2024 15:30:45 PM"

    BUGS
        - Does not support all strftime() specifiers.
        - Does not use the user�s actual locale settings.

    SEE ALSO

    INTERNALS
        This implementation uses an internal helper function, __strftime_locale(),
        to perform the actual formatting. The helper supports localization via
        the AmigaOS locale.library when available.

******************************************************************************/
{
    struct Locale *locale = __crt_get_locale();
    return __strftime_locale(s, maxsize, format, timeptr, locale);
}

