#ifndef UTILITY_DATE_H
#define UTILITY_DATE_H

/*
    (C) 1997 AROS - The Amiga Research OS

    $Id$

    Desc: ClockData
    Lang: English
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct ClockData
{
    UWORD sec;
    UWORD min;
    UWORD hour;
    UWORD mday;
    UWORD month;
    UWORD year;
    UWORD wday;
};

#endif /* UTILITY_DATE_H */
