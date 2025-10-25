/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.
*/

#ifndef _LOG_INTERN_H
#define _LOG_INTERN_H

#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <dos/datetime.h>

#include <devices/timer.h>

#define LOGMS_Flag_Private              24
#define LOGM_Flag_PrivateMask           (0xFF << LOGMS_Flag_Private)

#define LOGF_Flag_Private_STMPKrn       (0x1 << LOGMS_Flag_Private)
#define LOGF_Flag_Private_STMPTimer     (0x2 << LOGMS_Flag_Private)

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
    APTR                llh_Pool;
    struct MsgPort     *llh_MsgPort;            /* Target message port                  */
    ULONG               llh_MsgMask;            /* Mask of messages to send             */
};

struct LogResBase
{
    struct Library          lrb_Lib;
    APTR                    lrb_KernelBase;
    struct ExecBase         *lrb_ExecBase;
    struct UtilityBase      *lrb_UtilityBase;   /* for tags etc                         */
    struct Library          *lrb_DosBase;       /* for dos stuff                        */

    /* broadcast/service task */
    struct Task             *lrb_Task;
    struct MsgPort          *lrb_ServicePort;

    /* low memory handler */
    struct Interrupt        lrb_LowMemHandler;
    
    /* internal log data */
    struct LogResHandle     lrb_LRProvider;
    struct SignalSemaphore  lrb_ListenerLock;  /* Locks for non-reentrant stuff ..... */
    struct SignalSemaphore  lrb_ReentrantLock;
    struct timerequest      lrb_TimerIOReq;     /* Standard timer request               */
    struct List             lrb_Providers;      /* List of EventProviders               */
    struct List             lrb_Listeners;      /* List of EventListeners               */
    
    /* broadcast/service task data */
    BYTE                    lrb_sigTryDOS;
    BYTE                    lrb_sigTryTimer;
};

#endif /* _LOG_INTERN_H */
