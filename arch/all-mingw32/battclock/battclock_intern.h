/*
    Copyright  1995-2006, The AROS Development Team. All rights reserved.
    $Id: battclock_intern.h 24607 2006-08-05 15:39:09Z verhaegs $

    Desc: Internal data structures for battclock.resource and HIDD
    Lang: english
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif
#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
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
    __attribute__((stdcall)) void (*GetSystemTime)(SYSTEMTIME *lpSystemTime);
    __attribute__((stdcall)) ULONG (*SetSystemTime)(SYSTEMTIME *lpSystemTime);
};

struct BattClockBase
{
    struct Node	bb_Node;
    APTR Lib;
    struct KernelInterface *KernelIFace;
};

#endif //BATTCLOCK_INTERN_H
