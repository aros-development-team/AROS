/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <langinfo.h>
#include <locale.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <langinfo.h>

/* Return a borrowed weekday name in a shared 100-byte buffer, or an
 * empty literal when strftime returns zero. day_index must be 0..6
 * (Sunday first); nonzero abbreviated selects the short form. Calls
 * overwrite earlier weekday results. strftime uses the system locale;
 * names requiring 100 or more bytes reach its output-overrun defect.
 */
static const char* get_weekday_name(int day_index, int abbreviated) {
    static char buf[100];
    struct tm tm = {0};

    /* Set tm_wday to desired day and a known date to avoid issues */
    tm.tm_wday = day_index;

    /* Choose format */
    const char *fmt = abbreviated ? "%a" : "%A";

    /* Use a fixed date that matches tm_wday: e.g. 2025-06-22 is Sunday */
    tm.tm_year = 125;  // year 2025 since 1900
    tm.tm_mon = 5;     // June
    tm.tm_mday = 22 + day_index; // offset days to get correct weekday

    if (strftime(buf, sizeof(buf), fmt, &tm) == 0) {
        return "";
    }
    return buf;
}

/* Return the current local date as text in a shared 100-byte buffer,
 * or an empty literal when strftime returns zero. This is not a format
 * pattern. The time() failure value is passed on without checking.
 * The returned buffer is overwritten by the next call.
 */
static const char* get_date_format() {
    static char buf[100];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    if (strftime(buf, sizeof(buf), "%x", &tm) == 0) {
        return "";
    }
    return buf;
}

/* Return a borrowed suffix after the first dot in the locale name,
 * or the literal "unknown" when no name or suffix delimiter exists.
 * The setlocale query currently resets the C-library locale to C,
 * so this path returns "unknown" and changes locale state.
 */
static const char* get_codeset() {
    const char* loc = setlocale(LC_CTYPE, NULL);
    if (!loc) return "unknown";

    /* Usually locale strings look like: "fr_FR.UTF-8" */
    const char *dot = strchr(loc, '.');
    if (dot) {
        return dot + 1;
    }
    return "unknown";
}

/* Return the borrowed nonempty decimal-point string from localeconv,
 * or the literal "." if unavailable. No ownership is transferred.
 */
static const char* get_radixchar() {
    struct lconv *lc = localeconv();
    if (lc && lc->decimal_point && lc->decimal_point[0] != '\0') {
        return lc->decimal_point;
    }
    return ".";
}

/* Return the borrowed nonempty grouping separator from localeconv,
 * or an empty literal if unavailable. No ownership is transferred.
 */
static const char* get_thousep() {
    struct lconv *lc = localeconv();
    if (lc && lc->thousands_sep && lc->thousands_sep[0] != '\0') {
        return lc->thousands_sep;
    }
    return "";
}

/* Return AM text for nonzero am, PM text otherwise, in a shared
 * 10-byte buffer; return an empty literal when strftime returns zero.
 * Calls overwrite either earlier result. strftime uses the system
 * locale; text requiring 10 or more bytes reaches its overrun defect.
 */
static const char* get_am_pm(int am) {
    static char buf[10];
    struct tm tm = {0};

    tm.tm_hour = am ? 1 : 13;  // 1 AM or 1 PM
    if (strftime(buf, sizeof(buf), "%p", &tm) == 0) {
        return "";
    }
    return buf;
}

/* AROS "nl_langinfo" implementation */
/****************************************************************************
    NAME
        char * nl_langinfo(

    SYNOPSIS
        nl_item item)

    FUNCTION
        Return information for one of the implemented locale items.

    INPUTS
        item - CODESET, ABDAY_1, DAY_1, D_FMT, RADIXCHAR, THOUSEP,
               AM_STR or PM_STR. All other values return an empty
               string.

    RESULT
        Borrowed string; do not modify or free it. RADIXCHAR is
        "." and THOUSEP is empty. ABDAY_1 and DAY_1 select Sunday.
        Formatting failure also returns an empty string, so an
        empty result does not distinguish failure from valid data
        or an unsupported item. See BUGS before relying on results.

    NOTES
        Include <langinfo.h>. Copy the result before another call
        that uses the same buffer: weekday queries share one buffer,
        and AM_STR and PM_STR share another. D_FMT uses a separate
        buffer. Calls are not reentrant; synchronize shared use.

        Weekday and AM/PM text comes from strftime(), which uses
        the default system locale when available and English
        fallback strings otherwise. It does not use the C-library
        locale handle. D_FMT reads the current local date.

    BUGS
        CODESET calls setlocale(LC_CTYPE, NULL), which resets the
        C-library locale to "C" in this implementation; the result
        is then "unknown". D_FMT returns a date formatted as
        month/day/two-digit-year, not a format string. Time-read
        failure is not checked before the date is formatted.

        The strftime() dependency can write beyond the buffer when
        a weekday name occupies 100 or more bytes, or AM/PM text
        occupies 10 or more bytes. It also leaves opened system
        locale references unreleased. No reliable error indicator
        is provided for these defects.

    SEE ALSO
        nl_langinfo_l(), setlocale(), localeconv(), strftime()

*****************************************************************************/
char *nl_langinfo(nl_item item) {
    switch (item) {
        case CODESET:
            return (char *)get_codeset();

        case ABDAY_1:  /* Abbreviated Sunday */
            return (char *)get_weekday_name(0, 1);

        case DAY_1:    /* Full Sunday */
            return (char *)get_weekday_name(0, 0);

        case D_FMT:
            return (char *)get_date_format();

        case RADIXCHAR:
            return (char *)get_radixchar();

        case THOUSEP:
            return (char *)get_thousep();

        case AM_STR:
            return (char *)get_am_pm(1);

        case PM_STR:
            return (char *)get_am_pm(0);

        default:
            return "";
    }
}
