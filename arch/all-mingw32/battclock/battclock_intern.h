/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for Windows-hosted battclock.resource
    Lang: english
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

#include <exec/libraries.h>

#ifdef __x86_64__
#define __stdcall __attribute__((ms_abi))
#else
#define __stdcall __attribute__((stdcall))
#endif

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
    void  __stdcall (*GetSystemTime)(SYSTEMTIME *lpSystemTime);
    ULONG __stdcall (*SetSystemTime)(SYSTEMTIME *lpSystemTime);
};

struct BattClockBase
{
    struct Library bb_LibNode;
    APTR Lib;
    struct KernelInterface *KernelIFace;
};

#endif //BATTCLOCK_INTERN_H
