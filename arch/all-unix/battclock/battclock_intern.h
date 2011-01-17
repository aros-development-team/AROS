/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for battclock.resource and HIDD
    Lang: english
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

#include <time.h>

#include <exec/libraries.h>

struct BattclockInterface
{
    time_t (*time)(time_t *t);
    struct tm *(*localtime)(const time_t *timep);
};

struct BattClockBase
{
    struct Library bb_LibNode;
    APTR Lib;
    struct BattclockInterface *SysIFace;
};

#endif //BATTCLOCK_INTERN_H
