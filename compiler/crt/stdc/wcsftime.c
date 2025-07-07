/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <libraries/localestd.h>

#include <wchar.h>
#include <time.h>
/*
TODO: Implement localization support
#include <locale.h>
#include <libraries/locale.h>
#include <proto/locale.h>
*/

#define WADDS(st)  tmp=wcsftime(s,maxsize-size,(st),timeptr);break;

#define WADDN(a,b) tmp=wcsstrfnumb(s,maxsize-size,(a),(b));break;

#define WSTOR(c)   if(++size<=maxsize)*s++=(c);

#if 0
#define WSTR(a) \
(__localevec[LC_TIME-1]==NULL?wstrings[(a)-1]:GetLocaleWideStr(__localevec[LC_TIME-1],(a)))
#else
#define WSTR(a) (wstrings[(a)-1])
#endif

/* extern struct Locale *__localevec[]; */

/* All calendar wide strings */
static const wchar_t *wstrings[]=
{
    /* 0 */
    L"Sunday",L"Monday",L"Tuesday",L"Wednesday",L"Thursday",L"Friday",L"Saturday",
    /* 7 */
    L"Sun",L"Mon",L"Tue",L"Wed",L"Thu",L"Fri",L"Sat",
    /* 14 */
    L"January",L"February",L"March",L"April",L"May",L"June",
    L"July",L"August",L"September",L"October",L"November",L"December",
    /* 26 */
    L"Jan",L"Feb",L"Mar",L"Apr",L"May",L"Jun",L"Jul",L"Aug",L"Sep",L"Oct",L"Nov",L"Dec",
    /* 39 */
    L"",L"",
    /* 41 */
    L"AM",L"PM"
};

static size_t wcsstrfnumb(wchar_t *s, size_t maxsize, signed int places, size_t value)
{
    size_t size = 0;
    
    if (places > 1)
    {
        size = wcsstrfnumb(s, maxsize, places - 1, value / 10);
    }
    else if (value >= 10)
    {
        size = wcsstrfnumb(s, maxsize, places + 1, value / 10);
    }
    else
    {
        while ((places++ < -1) && (++size <= maxsize))
        {
            s[size - 1] = L' ';
        }
    }
    
    if (++size <= maxsize) s[size - 1] = (value % 10 + L'0');
    
    return size;
}

/*****************************************************************************

    NAME */

        size_t wcsftime(

/*  SYNOPSIS */
        wchar_t *s,
        size_t maxsize,
        const wchar_t *format,
        const struct tm *timeptr)

/*  FUNCTION
        Format time/date according to locale-dependent format.
        This is the wide character version of strftime().

    INPUTS
        s - buffer to store the formatted string
        maxsize - maximum number of wide characters to store
        format - format string with conversion specifiers
        timeptr - pointer to tm structure containing time/date

    RESULT
        Number of wide characters stored in s (excluding null terminator),
        or 0 if the result would not fit in maxsize characters.

    NOTES
        Function does not take localization into account at the moment.
        Conversion specifiers are the same as strftime().

    EXAMPLE
        wchar_t buffer[100];
        struct tm *timeinfo;
        time_t rawtime;
        
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        wcsftime(buffer, 100, L"Today is %A, %B %d, %Y", timeinfo);

    BUGS

    SEE ALSO
        strftime()

    INTERNALS

******************************************************************************/
{
    size_t size = 0, tmp;

    if (format == NULL || timeptr == NULL)
        return 0;

    while (*format)
    {
        if (*format == L'%')
        {
            tmp = 0;

            switch(*++format)
            {
                case L'a':
                    WADDS(WSTR(ABDAY_1+timeptr->tm_wday+1));
                case L'b':
                case L'h':
                    WADDS(WSTR(ABMON_1+timeptr->tm_mon+1));
                case L'c':
                    WADDS(L"%m/%d/%y");
                case L'd':
                    WADDN(2,timeptr->tm_mday);
                case L'e':
                    WADDN(-2,timeptr->tm_mday);
                case L'j':
                    WADDN(3,timeptr->tm_yday+1);
                case L'k':
                    WADDN(-2,timeptr->tm_hour);
                case L'l':
                    WADDN(-2,timeptr->tm_hour%12+(timeptr->tm_hour%12==0)*12);
                case L'm':
                    WADDN(2,timeptr->tm_mon+1);
                case L'p':
                    WADDS(WSTR(AM_STR+(timeptr->tm_hour>=12)));
                case L'r':
                    WADDS(L"%I:%M:%S %p");
                case L'w':
                    WADDN(1,timeptr->tm_wday);
                case L'x':
                    WADDS(L"%m/%d/%y %H:%M:%S");
                case L'y':
                    WADDN(2,timeptr->tm_year%100);
                case L'A':
                    WADDS(WSTR(DAY_1+timeptr->tm_wday+1));
                case L'B':
                    WADDS(WSTR(MON_1+timeptr->tm_mon+1));
                case L'C':
                    WADDS(L"%a %b %e %H:%M:%S %Y");
                case L'D':
                    WADDS(L"%m/%d/%y");
                case L'H':
                    WADDN(2,timeptr->tm_hour);
                case L'I':
                    WADDN(2,timeptr->tm_hour%12+(timeptr->tm_hour%12==0)*12);
                case L'M':
                    WADDN(2,timeptr->tm_min);
                case L'R':
                    WADDS(L"%H:%M");
                case L'S':
                    WADDN(2,timeptr->tm_sec);
                case L'T':
                case L'X':
                    WADDS(L"%H:%M:%S");
                case L'U':
                    WADDN(2,(timeptr->tm_yday+7-timeptr->tm_wday)/7);
                case L'W':
                    WADDN(2,(timeptr->tm_yday+7-(6+timeptr->tm_wday)%7)/7);
                case L'Y':
                    WADDN(4,timeptr->tm_year+1900);
                case L't':
                    WSTOR(L'\t');
                    break;
                case L'n':
                    WSTOR(L'\n');
                    break;
                case L'%':
                    WSTOR(L'%');
                    break;
                case L'\0':
                    format--;
                    break;
            }
            
            size += tmp;
            s    += tmp;
        }
        else
        {
            WSTOR(*format);
        }
        
        format++;
    }

    WSTOR(L'\0');

    if (size > maxsize)
    {
        s -= size;

        if (maxsize) /* Don't know if this is necessary, therefore it's here ;-) */
        {
            s[maxsize - 1] = L'\0';
        }

        size = 1;
    }

    return size - 1;
}
