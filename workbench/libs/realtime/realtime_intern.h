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

    struct SignalSemaphore rtb_Locks[RT_MAXLOCK];

    struct MinList   rtb_ConductorList;
};

#define  UtilityBase  (GPB(RTBase)->rtb_UtilityBase)
#define  SysBase      (GPB(RTBase)->rtb_SysBase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct RealTimeBase *, RTBase, 3, RealTime)

#endif /* REALTIME_INTERN_H */

