#ifndef LIBRARIES_REALTIME_H
#define LIBRARIES_REALTIME_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Includes for realtime.library
    Lang: English
*/


#ifndef   EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef   EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef   EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef   UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef   UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif

#ifndef   EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif


/* A conductor is an abstraction that represent a group of applications
   that want to be syncronized. */

/* This structure may ONLY be allocated by realtime.library and it's
   READ ONLY! */

struct Conductor
{
    struct  Node     cdt_Link;
    UWORD            cdt_Reserved0;
    struct  MinList  cdt_Players;         /* The players linked to this
					      conductor */
    ULONG            cdt_ClockTime;
    ULONG            cdt_StartTime;
    ULONG            cdt_ExternalTime;    /* Time from external synchronizer */
    ULONG            cdt_MaxExternalTime; 
    ULONG            cdt_Metronome;       /* Current musical time */
    UWORD            cdt_Reserved1;
    UWORD            cdt_Flags;           /* Conductor flags; see below */
    UBYTE            cdt_State;           /* Conductor state; see below */
    struct Task     *cdt_Barrier;         /* Private, don't touch */
    struct SignalSemaphore cdt_Lock;      /* Private, don't touch */
};

/* Conductor flags */

#define  CONDUCTB_EXTERNAL  0             /* Clock is externally driven */
#define  CONDUCTB_GOTTICK   1             /* First tick from external source
					     received */
#define  CONDUCTB_METROSET  2             /* Metronome defined */
#define  CONDUCTB_PRIVATE   3             /* This is a private conductor */

#define  CONDUCTF_EXTERNAL  (1 << CONDUCTB_EXTERNAL)
#define  CONDUCTF_GOTTICK   (1 << CONDUCTB_GOTTICK)
#define  CONDUCTF_METROSET  (1 << CONDUCTB_METROSET)
#define  CONDUCTF_PRIVATE   (1 << CONDUCTB_PRIVATE)

/* Conductor states */

#define  CONDSTATE_STOPPED  0             /* Clock is stopped */
#define  CONDSTATE_PAUSED   1             /* Clock is paused */
#define  CONDSTATE_LOCATE   2             /* Switch to RUNNING when ready */
#define  CONDSTATE_RUNNING  3             /* Clock is running */

/* Argument to SetConductorState() -- these are not real conductor states */

#define  CONDSTATE_METRIC     -1          /* Ask high node to locate */
#define  CONDSTATE_SHUTTLE    -2          /* Time changing without clock
					     running. */
#define  CONDSTATE_LOCATE_SET -3          /* Done locating */


/*****************************************************************************/


/* The Player structure is the link between an application and a conductor.
   This structure may ONLY be allocated by realtime.library and is READ
   ONLY! */

struct Player
{
    struct Node       pl_Link;
    BYTE              pl_Reserved0;
    BYTE              pl_Reserved1;
    struct Hook      *pl_Hook;
    struct Conductor *pl_Source;             /* This player's conductor */
    struct Task      *pl_Task;               /* Task to signal for alarm */
    LONG              pl_MetricTime;         /* Current time in players
						metric */
    LONG              pl_AlarmTime;          /* Time for alarm */
    void             *pl_UserData;
    UWORD             pl_PlayerID;
    UWORD             pl_Flags;              /* Player flags; see below */
};

/* Player flags */

#define  PLAYERB_READY      0
#define  PLAYERB_ALARMSET   1	             /* The alarm is set */
#define  PLAYERB_QUIET      2                /* This player is used for sync.
					        only */
#define  PLAYERB_CONDUCTED  3
#define  PLAYERB_EXTSYNC    4	             /* This player is the external
					        synchronizer */

#define  PLAYERF_READY      (1 << PLAYERB_READY)
#define  PLAYERF_ALARMSET   (1 << PLAYERB_ALARMSET)
#define  PLAYERF_QUIET      (1 << PLAYERB_QUIET)
#define  PLAYERF_CONDUCTED  (1 << PLAYERB_CONDUCTED)
#define  PLAYERF_EXTSYNC    (1 << PLAYERB_EXTSYNC)


/*****************************************************************************/


/* Tags */

enum { PLAYER_Base = TAG_USER + 64,
       PLAYER_Hook,
       PLAYER_Name,
       PLAYER_Priority,
       PLAYER_Conductor,
       PLAYER_Ready,
       PLAYER_AlarmSigTask,
       PLAYER_Conducted,
       PLAYER_AlarmSigBit,
       PLAYER_Quiet,
       PLAYER_UserData,
       PLAYER_ID,
       PLAYER_AlarmTime,
       PLAYER_Alarm,
       PLAYER_ExtSync,
       PLAYER_ErrorCode };

	   
/*****************************************************************************/


/* Hook message method types */

#define  PM_TICK      0
#define  PM_STATE     1
#define  PM_POSITION  2
#define  PM_SHUTTLE   3

/* Structure used by the methods PM_TICK, PM_POSITION and PM_SHUTTLE. */

struct pmTime
{
    ULONG  pmt_Method;                  /* The actual method */
    ULONG  pmt_Time;
};

/* Structure used by the method PM_STATE. */

struct pmState
{
    ULONG  pms_Method;                  /* The actual method */
    ULONG  pms_OldState;                /* The state previous to the state
					   change */
};


/*****************************************************************************/


enum { RT_CONDUCTORS = 0, RT_MAXLOCK };


/* Note that all fields are READ ONLY! */

struct RealTimeBase
{
    struct Library rtb_LibNode;
    UBYTE          rtb_Reserved0[2];
    
    ULONG          rtb_Time;
    ULONG          rtb_TimeFrac;
    UWORD          rtb_Reserved1;
    WORD           rtb_TickErr;
};

/* The actual length of a tick is: 1/TICK_FREQ + rtb_TickErr * 1e-9 */

/* These two are hardware dependent... */
#define  RealTime_TickErr_Min  -705
#define  RealTime_TickErr_Max   705


/* Error codes */

enum { RTE_NOMEMORY = 801,	/* Allocation of memory failed */
       RTE_NOCONDUCTOR,		/* Player has no conductor */
       RTE_NOTIMER,		/* Allocation of timer failed */
       RTE_PLAYING };		/* Unable to shuttle while playing */



#endif /* LIBRARIES_REALTIME_H */

