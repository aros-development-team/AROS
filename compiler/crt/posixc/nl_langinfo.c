/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <langinfo.h>
#include <locale.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <langinfo.h>

/* Get localized weekday name (full or abbreviated)
 * day_index: 0 = Sunday ... 6 = Saturday
 * abbreviated: 1 = abbreviated name, 0 = full name
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

/* Get date format string */
static const char* get_date_format() {
    static char buf[100];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    if (strftime(buf, sizeof(buf), "%x", &tm) == 0) {
        return "";
    }
    return buf;
}

/* Simplified charset name retrieval from locale string */
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

/* Simplified radix char from localeconv() */
static const char* get_radixchar() {
    struct lconv *lc = localeconv();
    if (lc && lc->decimal_point && lc->decimal_point[0] != '\0') {
        return lc->decimal_point;
    }
    return ".";
}

/* Simplified thousands separator from localeconv() */
static const char* get_thousep() {
    struct lconv *lc = localeconv();
    if (lc && lc->thousands_sep && lc->thousands_sep[0] != '\0') {
        return lc->thousands_sep;
    }
    return "";
}

/* Simplified AM/PM strings from strftime */
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
