/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include "locale_intern.h"

static const UWORD monthdays[12] =
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
static const UBYTE monthday[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

BOOL _getnum(LONG numchars,
    LONG * valPtr,
    ULONG * cPtr,
    CONST_STRPTR * fmtTemplatePtr,
    BOOL * checkEOFPtr,
    const struct Locale *locale,
    const struct Hook *getCharFunc, struct LocaleBase *LocaleBase);
#define get2num(x) _getnum(2, (x), &c, &fmtTemplate, &checkEOF, locale, getCharFunc, LocaleBase)
#define get4num(x) _getnum(4, (x), &c, &fmtTemplate, &checkEOF, locale, getCharFunc, LocaleBase)

/*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_LH4(BOOL, ParseDate,

/*  SYNOPSIS */
        AROS_LHA(const struct Locale    *, locale, A0),
        AROS_LHA(struct DateStamp *, date, A1),
        AROS_LHA(CONST_STRPTR      , fmtTemplate, A2),
        AROS_LHA(const struct Hook      *, getCharFunc, A3),

/*  LOCATION */
        struct LocaleBase *, LocaleBase, 27, Locale)

/*  FUNCTION
        This function will convert a stream of characters into an AmigaDOS
        DateStamp structure. It will obtain its characters from the
        getCharFunc callback hook, and the given formatting template will
        be used to direct the parse.

    INPUTS
        locale      -   the locale to use for the formatting
        date        -   where to put the converted date. If this is NULL,
                        then this function can be used to verify a date
                        string.
        fmtTemplate -   the date template used to direct the parse of the
                        data. The following FormatDate() formatting
                        controls can be used:
                          %a %A %b %B %d %e %h %H %I %m %M %p %S %y %Y

                        See FormatDate() autodoc for more information.
        getCharFunc -   A callback Hook which is used to read the data
                        from a stream. The hook is called with:

                        A0 - address of the Hook structure
                        A2 - locale pointer
                        A1 - NULL

                                    BTW: The AmigaOS autodocs which state that A1
                        gets locale pointer and A2 NULL are wrong!!

                        The read character should be returned in D0. Note
                        that this is a 32 bit character not an 8 bit
                        character. Return a NULL character if you reach the
                        end of the stream.

    RESULT
        TRUE    -   If the parse could be performed.
        FALSE   -   If the format of the data did not match the formatting
                    string.

    NOTES
        This has a few differences from the implementation in locale.library
        v38. In particular:
            - %p does not have to be at the end of the line.
            - %d and %e are not effectively the same: leading spaces are
              allowed before %e, but not before %d.

    EXAMPLE

    BUGS
        %p, %b, %A and probably others accept substrings and superstrings of
        valid strings.

    SEE ALSO
        FormatDate()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG c;
    LONG day = 0, month = 0, hour = 0, min = 0, sec = 0;
    LONG year = 1978;
    BOOL leap, ampm = FALSE, checkEOF = TRUE;
    if ((fmtTemplate == NULL)
        || (getCharFunc == NULL)
        || (locale == NULL) || (*fmtTemplate == '\0'))
        return FALSE;

#define GetChar()\
        AROS_UFC3(ULONG, getCharFunc->h_Entry, \
                AROS_UFCA(const struct Hook *, getCharFunc, A0), \
                AROS_UFCA(const struct Locale *, locale, A2), \
                AROS_UFCA(ULONG, 0, A1))

    while (*fmtTemplate)
    {
        /* Check for EOF if we leave the loop */
        checkEOF = TRUE;

        if (*fmtTemplate == '%')
        {
            UBYTE strOffs = 0;
            fmtTemplate++;

            switch (*fmtTemplate++)
            {
                /* abbreviated weekday name */
            case 'a':
                strOffs = 7;
                /* weekday name */
            case 'A':
                {
                    CONST_STRPTR dayStr[7];
                    BOOL dayOk[7];
                    ULONG i, a;

                    for (i = 0; i < 7; i++)
                    {
                        dayOk[i] = TRUE;
                        dayStr[i] = GetLocaleStr(locale, i + strOffs + 1);
                    }

                    c = GetChar();
                    while ((c != '\0') && (c != *fmtTemplate))
                    {
                        for (i = 0; i < 7; i++)
                        {
                            a = ConvToUpper(locale, *(dayStr[i])++);
                            c = ConvToUpper(locale, c);

                            if (dayOk[i] && a)
                                if (a != c)
                                    dayOk[i] = FALSE;
                        }
                        c = GetChar();
                    }

                    /* End of stream in wrong place, or invalid */
                    if (((c == '\0') && *fmtTemplate)
                        || (c != *fmtTemplate))
                        return FALSE;

                    /* If we didn't get a valid day, fail */
                    i = 0;
                    while ((i < 7) && !dayOk[i++])
                        ;
                    if ((i == 7) && !dayOk[6])
                        return FALSE;

                    if (*fmtTemplate)
                        fmtTemplate++;
                    checkEOF = FALSE;
                }
                break;          /* case 'A': */

                /* abbreviated month name */
            case 'b':
                /* abbreviated month name */
            case 'h':
                /* month name */
            case 'B':
                {
                    CONST_STRPTR monthStr[24];
                    BOOL monthOk[24];
                    ULONG i, a;

                    for (i = 0; i < 24; i++)
                    {
                        monthOk[i] = TRUE;
                        monthStr[i] =
                            GetLocaleStr(locale, i + strOffs + MON_1);
                    }

                    c = GetChar();
                    while ((c != '\0') && (c != *fmtTemplate))
                    {
                        for (i = 0; i < 24; i++)
                        {
                            a = ConvToUpper(locale, *monthStr[i]);
                            if (a != '\0')
                                monthStr[i]++;
                            c = ConvToUpper(locale, c);

                            if (monthOk[i])
                                if (a != c)
                                    monthOk[i] = FALSE;
                        }
                        c = GetChar();
                    }
                    for (i = 0; i < 24; i++)
                    {
                        if (monthOk[i] && *monthStr[i] == '\0')
                            month = i % 12 + 1;
                    }

                    /* If we didn't get a valid month, fail */
                    if (month == 0)
                        return FALSE;

                    /* End of stream in wrong place, or invalid */
                    if (((c == '\0') && *fmtTemplate)
                        || (c != *fmtTemplate))
                        return FALSE;

                    if (*fmtTemplate)
                        fmtTemplate++;
                    checkEOF = FALSE;

                    break;
                }               /* case 'B': */

                /* Day no */
            case 'd':
                day = 0;
                c = GetChar();
                if (!get2num(&day))
                    return FALSE;
                if (day-- == 0)
                    return FALSE;

                break;
                /* Day no., leading spaces. */
            case 'e':
                day = 0;
                c = GetChar();
                while (IsSpace(locale, c))
                    c = GetChar();
                if (!get2num(&day))
                    return FALSE;
                if (day-- == 0)
                    return FALSE;

                break;

                /* hour 24-hr style */
            case 'H':
                ampm = FALSE;
                c = GetChar();
                if (!get2num(&hour))
                    return FALSE;
                if (hour > 23)
                    return FALSE;
                break;

                /* hour 12-hr style */
            case 'I':
                c = GetChar();
                if (!get2num(&hour))
                    return FALSE;
                if (hour > 11)
                    return FALSE;
                break;

                /* month num */
            case 'm':
                c = GetChar();
                if (!get2num(&month))
                    return FALSE;
                if ((month > 12) || (month == 0))
                    return FALSE;
                break;

                /* minutes */
            case 'M':
                c = GetChar();
                if (!get2num(&min))
                    return FALSE;

                if (min > 59)
                    return FALSE;
                break;

                /* AM or PM string */
            case 'p':
                {
                    CONST_STRPTR amStr, pmStr;
                    BOOL amOk = TRUE, pmOk = TRUE;
                    ULONG a, b;
                    amStr = GetLocaleStr(locale, AM_STR);
                    pmStr = GetLocaleStr(locale, PM_STR);

                    c = GetChar();
                    while ((c != '\0') && (c != *fmtTemplate))
                    {
                        a = ConvToUpper(locale, *amStr++);
                        b = ConvToUpper(locale, *pmStr++);
                        c = ConvToUpper(locale, c);

                        if (amOk && a)
                            if (a != c)
                                amOk = FALSE;

                        if (pmOk && b)
                            if (b != c)
                                pmOk = FALSE;

                        c = GetChar();
                    }

                    /* End of stream in wrong place, or invalid */
                    if (c != *fmtTemplate)
                        return FALSE;

                    /* Check whether we got AM or PM */
                    ampm = pmOk;

                    if (*fmtTemplate)
                        fmtTemplate++;
                    checkEOF = FALSE;
                    break;
                }

                /* the number of seconds */
            case 'S':
                c = GetChar();
                if (!get2num(&sec))
                    return FALSE;
                if (sec > 59)
                    return FALSE;
                break;

                /* the year using two or four digits */
            case 'y':
                c = GetChar();
                if (!get4num(&year))
                    return FALSE;

                if (year >= 100 && year < 1978)
                    return FALSE;
                if (year < 78)
                    year += 100;
                if (year < 1900)
                    year += 1900;
                break;

                /* the year using four digits */
            case 'Y':
                c = GetChar();
                if (IsDigit(locale, c) == FALSE)
                    return FALSE;
                year = (c - '0') * 1000;

                c = GetChar();
                if (IsDigit(locale, c) == FALSE)
                    return FALSE;
                year += (c - '0') * 100;

                c = GetChar();
                if (IsDigit(locale, c) == FALSE)
                    return FALSE;
                year += (c - '0') * 10;

                c = GetChar();
                if (IsDigit(locale, c) == FALSE)
                    return FALSE;
                year += (c - '0');

                if (year < 1978)
                    return FALSE;
                break;

            default:
                return FALSE;
                break;
            } /* switch() */
        } /* if (char == '%') */
        else
        {
            c = GetChar();
            if (c != *fmtTemplate++)
                return FALSE;
        }
    } /* while (*fmtTemplate) */

    /* Reached end of fmtTemplate, end of input stream? */
    if (checkEOF)
        if ((GetChar() != 0))
            return FALSE;

    /* Is this year a leap year ? */
    leap = (((year % 400) == 0) ||
        (((year % 4) == 0) && !((year % 100) == 0)));

    /* Sanity check */
    if (month != 0 && day >=
        (monthday[month - 1] + ((leap && (month == 2)) ? 1 : 0)))
    {
        return FALSE;
    }

    if (date)
    {
        /* Add the days for all years (without leap years) */
        day += (year - 1978) * 365;

        year--;

        /* Add leap years */
        day += ((year / 4) - (year / 100) + (year / 400) - (494 - 19 + 4));

        /* Add days of months */
        day += monthdays[month - 1];

        /*
           in monthdays, February has 28 days. Correct this in
           leap years if month is >= March.
         */

        if (leap && (month >= 3))
            day++;

        date->ds_Days = day;

        date->ds_Minute = hour * 60 + min;
        if ((hour < 12) && ampm)
            date->ds_Minute += 720;
        date->ds_Tick = sec * TICKS_PER_SECOND;
    }
    return TRUE;

    AROS_LIBFUNC_EXIT
}


BOOL _getnum(LONG numchars,
    LONG * valPtr,
    ULONG * cPtr,
    CONST_STRPTR * fmtTemplatePtr,
    BOOL * checkEOFPtr,
    const struct Locale * locale,
    const struct Hook * getCharFunc, struct LocaleBase * LocaleBase)
{
    LONG val;
    ULONG c;

    c = *cPtr;
    //*c = GetChar();
    if (IsDigit(locale, c) == FALSE)
        return FALSE;

    val = c - '0';

    while (--numchars >= 1)
    {
        c = GetChar();
        if (IsDigit(locale, c))
            val = val * 10 + c - '0';
        else
        {
            *cPtr = c;
            if (c != **fmtTemplatePtr)
                return FALSE;
            if (c == '\0')
                *checkEOFPtr = FALSE;
            else
                (*fmtTemplatePtr)++;

            break;
        }
    }

    *valPtr = val;

    return TRUE;
}
