/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/ports.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

/* Minimum interval between hook driven recomputes (about a vertical blank) */
#define CGXV_HOOK_INTERVAL  8000

static void cgxv_CloseTimer(struct IntCGXVBase *CGXVideoBase)
{
    if (CGXVideoBase->stctx.TimerBase)
    {
        CloseDevice((struct IORequest *)CGXVideoBase->timerIO);
        CGXVideoBase->stctx.TimerBase = NULL;
    }
    if (CGXVideoBase->timerIO)
    {
        DeleteIORequest((struct IORequest *)CGXVideoBase->timerIO);
        CGXVideoBase->timerIO = NULL;
    }
    if (CGXVideoBase->timerMP)
    {
        DeleteMsgPort(CGXVideoBase->timerMP);
        CGXVideoBase->timerMP = NULL;
    }
}

/*
 * timer.device is only needed for ReadEClock(), a plain call: the port
 * never receives anything, so it gives its signal back right away.
 */
static void cgxv_OpenTimer(struct IntCGXVBase *CGXVideoBase)
{
    CGXVideoBase->timerMP = CreateMsgPort();
    if (!CGXVideoBase->timerMP)
        return;

    FreeSignal(CGXVideoBase->timerMP->mp_SigBit);
    CGXVideoBase->timerMP->mp_SigBit = -1;
    CGXVideoBase->timerMP->mp_Flags  = (CGXVideoBase->timerMP->mp_Flags & ~PF_ACTION) | PA_IGNORE;

    CGXVideoBase->timerIO = (struct timerequest *)CreateIORequest(CGXVideoBase->timerMP,
                                                                  sizeof(struct timerequest));
    if (CGXVideoBase->timerIO &&
        !OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)CGXVideoBase->timerIO, 0))
    {
        CGXVideoBase->stctx.TimerBase   = CGXVideoBase->timerIO->tr_node.io_Device;
        CGXVideoBase->stctx.MinInterval = CGXV_HOOK_INTERVAL;
        return;
    }

    cgxv_CloseTimer(CGXVideoBase);
}

static int cgxv_init(struct IntCGXVBase *CGXVideoBase)
{
    CGXVideoBase->attrbases[0].interfaceID = IID_Hidd_BitMap;
    CGXVideoBase->attrbases[0].attrBase    = &CGXVideoBase->cgxv_HiddBitMapAttrBase;
    CGXVideoBase->attrbases[1].interfaceID = IID_Hidd_Display;
    CGXVideoBase->attrbases[1].attrBase    = &CGXVideoBase->cgxv_HiddDisplayAttrBase;
    CGXVideoBase->attrbases[2].interfaceID = NULL;

    if (!OOP_ObtainAttrBases(CGXVideoBase->attrbases))
        return FALSE;

    CGXVideoBase->cgxv_HiddBitMapBase  = OOP_GetMethodID(IID_Hidd_BitMap, 0);
    CGXVideoBase->cgxv_HiddDisplayBase = OOP_GetMethodID(IID_Hidd_Display, 0);
    CGXVideoBase->basebm = OOP_FindClass(CLID_Hidd_BitMap);

    InitSemaphore(&CGXVideoBase->handleLock);
    NEWLIST((struct List *)&CGXVideoBase->handles);

    cgxv_OpenTimer(CGXVideoBase);

    CGXVideoBase->notifyActive = st_StartCloseNotify(&CGXVideoBase->notify,
                                                     cgxv_CloseNotify, CGXVideoBase);
    D(bug("[CGXVideo] %s: close notify %s\n", __func__,
          CGXVideoBase->notifyActive ? "active" : "unavailable"));

    return TRUE;
}

static int cgxv_expunge(struct IntCGXVBase *CGXVideoBase)
{
    if (CGXVideoBase->notifyActive)
    {
        st_EndCloseNotify(&CGXVideoBase->notify);
        CGXVideoBase->notifyActive = FALSE;
    }
    cgxv_CloseTimer(CGXVideoBase);
    OOP_ReleaseAttrBases(CGXVideoBase->attrbases);

    return TRUE;
}

ADD2INITLIB(cgxv_init, 0);
ADD2EXPUNGELIB(cgxv_expunge, 0);
