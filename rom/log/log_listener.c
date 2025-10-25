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

AROS_LH2(struct logListenerHook *, logAddListener,
         AROS_LHA(struct MsgPort *, mp, A1),
         AROS_LHA(ULONG, msgmask, D0),
         LIBBASETYPEPTR, LIBBASE, 17, log)
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

AROS_LH1(VOID, logRemListener,
         AROS_LHA(APTR, listener, A0),
         LIBBASETYPEPTR, LIBBASE, 18, log)
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

AROS_LH3(VOID, logEventBroadcast,
         AROS_LHA(UBYTE, ehmt, D0),
         AROS_LHA(APTR, param1, A0),
         AROS_LHA(APTR, param2, A1),
         LIBBASETYPEPTR, LIBBASE, 19, log)
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

AROS_LH2(void, logDeleteBroadcast,
         AROS_LHA(APTR, listener, A0),
         AROS_LHA(APTR, bMsg, A1),
         LIBBASETYPEPTR, LIBBASE, 20, log)
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
    struct logListenerHook *llh = (struct logListenerHook *)listener;
    FreeVecPooled(llh->llh_Pool, bMsg);

    AROS_LIBFUNC_EXIT
}
