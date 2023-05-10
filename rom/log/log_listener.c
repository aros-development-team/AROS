/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#define __NOLIBBASE__
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

AROS_LH2(struct logListenerHook *, logAddListener,
         AROS_LHA(struct MsgPort *, mp, A1),
         AROS_LHA(ULONG, msgmask, D0),
         LIBBASETYPEPTR, LIBBASE, 17, log)
{
    AROS_LIBFUNC_INIT

    struct logListenerHook *llh = NULL;

    if(mp)
    {
        ObtainSemaphore(&LIBBASE->lrb_ReentrantLock);
        if((llh = AllocVecPooled(LIBBASE->lrb_LRProvider.lrh_Pool, sizeof(struct logListenerHook))))
        {
            llh->llh_MsgPort = mp;
            llh->llh_MsgMask = msgmask;
            llh->llh_Node.ln_Pred = NULL;
            llh->llh_Node.ln_Pred = NULL;

            AddTail(&LIBBASE->lrb_Listeners, &llh->llh_Node);
        }
        ReleaseSemaphore(&LIBBASE->lrb_ReentrantLock);
    }
    return(llh);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, logEventBroadcast,
         AROS_LHA(ULONG, ehmt, D0),
         AROS_LHA(APTR, param1, A0),
         AROS_LHA(APTR, param2, A1),
         LIBBASETYPEPTR, LIBBASE, 19, log)
{
    AROS_LIBFUNC_INIT

    struct logListenerHook *llh;
    ULONG msgmask = (1L<<ehmt);

    ObtainSemaphore(&LIBBASE->lrb_ReentrantLock);

    llh = (struct logListenerHook *) LIBBASE->lrb_Listeners.lh_Head;
    while(llh->llh_Node.ln_Succ)
    {
        if(llh->llh_MsgMask & msgmask)
        {
            // TODO:
        }
        llh = (struct logListenerHook *) llh->llh_Node.ln_Succ;
    }

    ReleaseSemaphore(&LIBBASE->lrb_ReentrantLock);

    AROS_LIBFUNC_EXIT
}
