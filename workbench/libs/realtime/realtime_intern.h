/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

    BPTR             rtb_SegList;
    struct ExecBase *rtb_SysBase;
    struct Library  *rtb_UtilityBase;
    struct Library  *rtb_DOSBase;

    struct SignalSemaphore rtb_Locks[RT_MAXLOCK];

    struct MinList   rtb_ConductorList;

    struct Interrupt rtb_VBlank;
    struct Task     *rtb_PulseTask;
};

#define  UtilityBase  (GPB(RealTimeBase)->rtb_UtilityBase)
#define  SysBase      (GPB(RealTimeBase)->rtb_SysBase)
#define  DOSBase      (GPB(RealTimeBase)->rtb_DOSBase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct RealTimeBase *, RealTimeBase, 3, RealTime)

#endif /* REALTIME_INTERN_H */

