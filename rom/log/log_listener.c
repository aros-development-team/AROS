/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/log.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <resources/log.h>

#include "log_intern.h"

#include LC_LIBDEFS_FILE

#define DOSBase LIBBASE->lrb_DosBase
#define TimerBase LIBBASE->lrb_TimerIOReq.tr_node.io_Device
#define UtilityBase LIBBASE->lrb_UtilityBase

/*
 * msgmask == (LOGF_Flag_Type_* << LOGMS_Flag_Type) | EHMF_*
 * where :
 *     LOGF_Flag_Type_* - the event types to listen for
 *     EHMF_*                      - the events to listen for.
 */

/*****************************************************************************

    NAME */
        AROS_LH2(struct logListenerHook *, logAddListener,

/*  SYNOPSIS */
                 AROS_LHA(struct MsgPort *, mp, A1),
                 AROS_LHA(ULONG, msgmask, D0),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 17, log)
/*  FUNCTION
        Register a message port for selected log notifications.

    INPUTS
        mp - Live message port, or NULL. Keep it valid until registration
             has ended and all queued notifications have been handled.
        msgmask - Event bits EHMF_* combined with LOGF_Flag_Type_* bits.

    RESULT
        A listener handle for logDeleteBroadcast() and logRemListener(),
        or NULL for a NULL port or listener-handle allocation failure.

    NOTES
        A notification matches only if msgmask contains its event bit and
        all of its entry-type bits. To receive all defined types and events,
        use LOGF_Flag_Type_All | EHMF_ALLEVENTS. Existing entries are not
        replayed. Registration order follows the port's priority at this call.

        Messages have type struct LogResBroadcastMsg. Remove them from the
        port with GetMsg(), then dispose of each with logDeleteBroadcast()
        using this listener handle. Do not ReplyMsg() or FreeVec() them.
        The target pointer is borrowed, not pinned by the message.
        Call from task context; registration may wait on a semaphore.

    BUGS
        Creation of the listener's message pool is not checked. A non-NULL
        handle can therefore contain an unusable pool. Entry lifetime is
        not extended for queued messages; removal targets may already be
        freed when a listener receives them.

    SEE ALSO
        logDeleteBroadcast(), logRemListener(), logEventBroadcast()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct logListenerHook *llh = NULL;

    if(mp)
    {
        struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
        if((llh = AllocVecPooled(LIBBASE->lrb_LRProvider.lrh_Pool, sizeof(struct logListenerHook))))
        {
            llh->llh_Node.ln_Type = NT_LISTENER;
            llh->llh_Node.ln_Pri = mp->mp_Node.ln_Pri;
            llh->llh_Node.ln_Pred = NULL;
            llh->llh_Node.ln_Succ = NULL;
            llh->llh_MsgPort = mp;
            llh->llh_MsgMask = msgmask;
            llh->llh_Pool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC, 1024, 1024);

            ObtainSemaphore(&LIBBASE->lrb_ListenerLock);
            Enqueue(&LIBBASE->lrb_Listeners, &llh->llh_Node);
            ReleaseSemaphore(&LIBBASE->lrb_ListenerLock);
        }
    }
    return(llh);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(VOID, logRemListener,

/*  SYNOPSIS */
                 AROS_LHA(APTR, listener, A0),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 18, log)
/*  FUNCTION
        Unregister a listener and delete its message pool.

    INPUTS
        listener - Live handle returned by logAddListener(). NULL and
                   previously removed handles are invalid.

    RESULT
        None. The handle and all allocations from its pool become invalid.

    NOTES
        The listener lock serializes unregistration with broadcasts. It
        does not drain the destination port or wait for consumers. Before
        removal, coordinate publishers and consumers so no new notification
        can arrive and no queued or retained message remains. Dispose of
        messages with logDeleteBroadcast() while the handle is still valid.
        The message port itself remains caller-owned.

        Call from task context; acquiring the listener lock may wait.

    BUGS
        Deleting the pool invalidates outstanding messages even if they
        remain linked on the destination port. The API has no separate
        stop-delivery operation that preserves the pool for draining.
        Draining the port while broadcasts continue is not a safe shutdown
        protocol.

    SEE ALSO
        logAddListener(), logDeleteBroadcast()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
    struct logListenerHook *llh = (struct logListenerHook *)listener;
    ObtainSemaphore(&LIBBASE->lrb_ListenerLock);
    Remove(&llh->llh_Node);
    ReleaseSemaphore(&LIBBASE->lrb_ListenerLock);
    DeletePool(llh->llh_Pool);
    FreeVecPooled(LIBBASE->lrb_LRProvider.lrh_Pool, llh);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(VOID, logEventBroadcast,

/*  SYNOPSIS */
                 AROS_LHA(UBYTE, ehmt, D0),
                 AROS_LHA(APTR, param1, A0),
                 AROS_LHA(APTR, param2, A1),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 19, log)
/*  FUNCTION
        Enqueue a notification for each matching listener.

    INPUTS
        ehmt - Event index EHMB_*, not an EHMF_* mask.
        param1 - Live log entry pointer used to read its type flags and
                 stored as the notification's lrbm_Target.
        param2 - Reserved; currently ignored.

    RESULT
        None. No delivery count or allocation-failure status is returned.

    NOTES
        Matching requires all bits in
            (entryFlags & LOGM_Flag_TypeMask) | (1L << ehmt)
        to be present in the listener mask. Each message's lrbm_MsgType is
        the event mask, not the event index. Messages are allocated from
        the corresponding listener pool and sent asynchronously with PutMsg().

        The caller must keep param1 valid while this function reads it.
        The message does not retain or copy the entry. In particular, an
        EHMF_REMENTRY target is an identity value, not a safe-to-dereference
        entry. Call from task context; the listener semaphore may block.

    BUGS
        Message allocation failure is not checked before dereferencing
        the result. All event types require a live entry in param1 because
        the implementation reads entry flags unconditionally; NULL is not
        valid even for START or STOP events.

    SEE ALSO
        logAddListener(), logDeleteBroadcast()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
    struct logListenerHook *llh;
    struct logEntryPrivate *leP = (struct logEntryPrivate *)((IPTR)param1 - offsetof(struct logEntryPrivate, le_Node));
    ULONG msgMatch = (leP->lectx_Level & LOGM_Flag_TypeMask) | (1L << ehmt);

    ObtainSemaphore(&LIBBASE->lrb_ListenerLock);
    ForeachNode(&LIBBASE->lrb_Listeners, llh)
    {
        if ((llh->llh_MsgMask & msgMatch) == msgMatch)
        {
            struct LogResBroadcastMsg *lrBMsg = AllocVecPooled(llh->llh_Pool, sizeof(struct LogResBroadcastMsg));
            lrBMsg->lrbm_MsgType = (1L << ehmt);
            lrBMsg->lrbm_Target = param1;
            lrBMsg->lrbm_Msg.mn_Node.ln_Pred = NULL;
            lrBMsg->lrbm_Msg.mn_Node.ln_Succ = NULL;
            PutMsg(llh->llh_MsgPort, (struct Message *)lrBMsg);
        }
    }
    ReleaseSemaphore(&LIBBASE->lrb_ListenerLock);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(void, logDeleteBroadcast,

/*  SYNOPSIS */
                 AROS_LHA(APTR, listener, A0),
                 AROS_LHA(APTR, bMsg, A1),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 20, log)
/*  FUNCTION
        Free one notification received by a listener.

    INPUTS
        listener - Live handle for the listener that received bMsg.
        bMsg - Notification already removed from that listener's port,
               not previously freed.

    RESULT
        None. bMsg becomes invalid.

    NOTES
        This frees only the notification, not its target entry. Use the
        matching listener because the message belongs to its private pool.
        Do not call after logRemListener(), which deletes that pool. The
        function performs no port removal, ReplyMsg(), origin check or
        listener locking; coordinate it with listener teardown.

    SEE ALSO
        logAddListener(), logRemListener()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
    struct logListenerHook *llh = (struct logListenerHook *)listener;
    FreeVecPooled(llh->llh_Pool, bMsg);

    AROS_LIBFUNC_EXIT
}
