/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#ifndef _LOG_INTERN_H
#define _LOG_INTERN_H

#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <dos/datetime.h>

#include <devices/timer.h>

#define NT_LISTENER                     (1)
#define NT_PROVIDER                     (2)
#define NT_LOGENTRY                     (3)
    
#define LOGMS_Flag_Private              24
#define LOGM_Flag_PrivateMask           (0xFF << LOGMS_Flag_Private)

#define LOGRESENTRYCTX                                                                          \
    STRPTR              lectx_Originator;       /* Taskname */                                  \
    ULONG               lectx_Flags;            /* RC: 0=Note, 5=Warn, 10=Error, 20=Fail */

#define lectx_Level     lectx_Flags

#define LOGRESLOGENTRY                                                                          \
    struct Node         le_Node;                /* Node linkage */                              \
    struct DateStamp    le_DateStamp;           /* Date Stamp (if DOS available) */             \
    LOGRESENTRYCTX                                                                              \
    ULONG               le_eid;                                                                 \
    STRPTR              le_Entry;               /* Actual error message */

struct logEntry
{
    LOGRESLOGENTRY
};

struct logEntryPrivate
{
    struct Node         lep_Node;
    APTR                lep_Producer;
    LOGRESLOGENTRY
};

struct LogResHandle
{
    struct Node         lrh_Node;
    APTR                lrh_Pool;
    struct List         lrh_Entries;
};

struct logRDF
{
    ULONG               rdf_Len;
    STRPTR              rdf_Buf;
};

struct logListenerHook
{
    struct Node         llh_Node;               /* Node linkage                         */
    struct MsgPort     *llh_MsgPort;            /* Target message port                  */
    ULONG               llh_MsgMask;            /* Mask of messages to send             */
};

struct logEvent
{
    struct Message      lev_Msg;                /* Intertask communication message      */
    UWORD               lev_Event;              /* Event number as specified above      */
    APTR                lev_Param1;             /* Parameter 1 for event                */
    APTR                lev_Param2;             /* Parameter 2                          */
};

struct logEventInternal
{
    struct Node         levi_Node;              /* Node linkage                         */
    struct logEvent     levi_EventNote;         /* Encapsulated logEvent                */
};

struct LogResBase
{
    struct Library          lrb_Lib;
    struct UtilityBase      *lrb_UtilityBase;   /* for tags etc                         */
    struct Library          *lrb_DosBase;       /* for dos stuff                        */
    struct Task             *lrb_Task;
    struct MsgPort          *lrb_ServicePort;
    struct Interrupt        lrb_LowMemHandler;
    struct LogResHandle     lrb_LRProvider;
    struct SignalSemaphore  lrb_ReentrantLock;  /* Lock for non-reentrant stuff         */
    struct timerequest      lrb_TimerIOReq;     /* Standard timer request               */
    struct List             lrb_Providers;      /* List of EventProviders               */
    struct List             lrb_Listeners;      /* List of EventListeners               */
};

#endif /* _LOG_INTERN_H */
