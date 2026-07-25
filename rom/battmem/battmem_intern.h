/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Internal data structures for battmem.resource
*/

#ifndef BATTMEM_INTERN_H
#define BATTMEM_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

struct BattMemBase
{
    struct Library          bm_LibNode;
    struct SignalSemaphore  bm_Semaphore;
    /* battclock.resource holds the battery backed up memory itself */
    struct Library         *bm_BattClockBase;
};

#endif /* BATTMEM_INTERN_H */
