/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for battclock.resource and HIDD
    Lang: english
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

#include <exec/libraries.h>

typedef struct _SYSTEMTIME
{
    UWORD wYear;
    UWORD wMonth;
    UWORD wDayOfWeek;
    UWORD wDay;
    UWORD wHour;
    UWORD wMinute;
    UWORD wSecond;
    UWORD wMilliseconds;
} SYSTEMTIME,  *PSYSTEMTIME;

struct KernelInterface
{
    __attribute__((stdcall)) void (*GetSystemTime)(SYSTEMTIME *lpSystemTime);
    __attribute__((stdcall)) ULONG (*SetSystemTime)(SYSTEMTIME *lpSystemTime);
};

struct BattClockBase
{
    struct Library bb_LibNode;
    APTR Lib;
    struct KernelInterface *KernelIFace;
};

#endif //BATTCLOCK_INTERN_H
