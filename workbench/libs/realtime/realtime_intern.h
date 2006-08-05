/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef REALTIME_INTERN_H
#define REALTIME_INTERN_H

#ifndef   LIBRARIES_REALTIME_H
#include <libraries/realtime.h>
#endif

#ifndef   EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef   EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef   EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef   EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

#ifndef   EXEC_TASKS
#include <exec/tasks.h>
#endif

#ifndef   EXEC_INTERRUPTS
#include <exec/interrupts.h>
#endif

#include <dos/dos.h>

#define  GPB(x)  ((struct internal_RealTimeBase *)x)

/* Note that all fields are READ ONLY! */

struct internal_RealTimeBase
{
    struct Library   rtb_LibNode;
    UBYTE            rtb_Reserved0[2];
    
    ULONG            rtb_Time;
    ULONG            rtb_TimeFrac;
    UWORD            rtb_Reserved1;
    WORD             rtb_TickErr;

    struct SignalSemaphore rtb_Locks[RT_MAXLOCK];

    struct MinList   rtb_ConductorList;

    struct Interrupt rtb_VBlank;
    struct Task     *rtb_PulseTask;
};

#endif /* REALTIME_INTERN_H */

