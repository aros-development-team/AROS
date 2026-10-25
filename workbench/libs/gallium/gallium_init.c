/*
    Copyright (C) 2010-2019, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>

#include <dos/var.h>
#include <exec/ports.h>

#include <string.h>

#include LC_LIBDEFS_FILE
#include "gallium_intern.h"

CONST_STRPTR softpipe_str = "softpipe";

static void gallium_CloseTimer(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->stctx.TimerBase)
    {
        CloseDevice((struct IORequest *)LIBBASE->timerIO);
        LIBBASE->stctx.TimerBase = NULL;
    }
    if (LIBBASE->timerIO)
    {
        DeleteIORequest((struct IORequest *)LIBBASE->timerIO);
        LIBBASE->timerIO = NULL;
    }
    if (LIBBASE->timerMP)
    {
        DeleteMsgPort(LIBBASE->timerMP);
        LIBBASE->timerMP = NULL;
    }
}

/*
 * timer.device is only needed for ReadEClock() (surface hook rate limit),
 * a plain call: the port never receives anything, so it gives its signal
 * back right away (as cgxvideo.library does).
 */
static void gallium_OpenTimer(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->timerMP = CreateMsgPort();
    if (!LIBBASE->timerMP)
        return;

    FreeSignal(LIBBASE->timerMP->mp_SigBit);
    LIBBASE->timerMP->mp_SigBit = -1;
    LIBBASE->timerMP->mp_Flags  = (LIBBASE->timerMP->mp_Flags & ~PF_ACTION) | PA_IGNORE;

    LIBBASE->timerIO = (struct timerequest *)CreateIORequest(LIBBASE->timerMP,
                                                             sizeof(struct timerequest));
    if (LIBBASE->timerIO &&
        !OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)LIBBASE->timerIO, 0))
    {
        LIBBASE->stctx.TimerBase   = LIBBASE->timerIO->tr_node.io_Device;
        LIBBASE->stctx.MinInterval = GALLIUM_SFB_HOOK_INTERVAL;
        return;
    }

    gallium_CloseTimer(LIBBASE);
}

static int Init(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->gfxAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gfx);
    if (!LIBBASE->gfxAttrBase)
        return FALSE;

    LIBBASE->bmAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_BitMap);
    if (!LIBBASE->bmAttrBase)
        return FALSE;

    LIBBASE->galliumAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium);
    if (!LIBBASE->galliumAttrBase)
        return FALSE;

    LIBBASE->displayAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Display);
    if (!LIBBASE->displayAttrBase)
        return FALSE;

    LIBBASE->pixfmtAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_PixFmt);
    if (!LIBBASE->pixfmtAttrBase)
        return FALSE;

    LIBBASE->basegallium = OOP_FindClass(CLID_Hidd_Gallium);
    LIBBASE->fallback = (char *)softpipe_str;
    LIBBASE->fallbackmodule = NULL;

    /* Which software rasteriser to use when the display has no gallium
       driver of its own. (llvmpipe, softpipe) */
    {
        char buf[64];

        if (GetVar("SYS/Gallium.default", buf, sizeof(buf),
                   GVF_GLOBAL_ONLY | LV_VAR) > 0)
        {
            char *sel = AllocVec(strlen(buf) + 1, MEMF_PUBLIC);

            if (sel)
            {
                strcpy(sel, buf);
                LIBBASE->fallback = sel;
            }
        }
    }

    /* cache method id's that we use ..  */
    LIBBASE->galliumMId_UpdateRect = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_UpdateRect);
    LIBBASE->galliumMId_DisplayResource = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DisplayResource);
    LIBBASE->galliumMId_DisplayResourceRP = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DisplayResourceRP);
    LIBBASE->galliumMId_ResourceFromBitMap = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_ResourceFromBitMap);
    LIBBASE->bmMethodBase = OOP_GetMethodID(IID_Hidd_BitMap, 0);
    LIBBASE->displayMethodBase = OOP_GetMethodID(IID_Hidd_Display, 0);
    LIBBASE->basebm = OOP_FindClass(CLID_Hidd_BitMap);

    /* Scan-out framebuffers */
    InitSemaphore(&LIBBASE->sfbLock);
    NEWLIST((struct List *)&LIBBASE->sfbList);
    gallium_OpenTimer(LIBBASE);
    LIBBASE->sfbNotifyActive = st_StartCloseNotify(&LIBBASE->sfbNotify,
                                                   gallium_sfb_CloseNotify, LIBBASE);
    D(bug("[Gallium] %s: close notify %s\n", __func__,
          LIBBASE->sfbNotifyActive ? "active" : "unavailable"));

    return TRUE;
}

static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sfbNotifyActive)
    {
        st_EndCloseNotify(&LIBBASE->sfbNotify);
        LIBBASE->sfbNotifyActive = FALSE;
    }
    gallium_CloseTimer(LIBBASE);

    if (LIBBASE->pixfmtAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_PixFmt);

    if (LIBBASE->displayAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Display);

    if (LIBBASE->galliumAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    if (LIBBASE->bmAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_BitMap);

    if (LIBBASE->gfxAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gfx);

    if (LIBBASE->fallbackmodule)
        CloseLibrary(LIBBASE->fallbackmodule);

    if ((CONST_STRPTR)LIBBASE->fallback != softpipe_str)
        FreeVec(LIBBASE->fallback);

    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);

ADD2LIBS((STRPTR)"gallium.hidd", 0, static struct Library *, GalliumHiddBase);
ADD2LIBS((STRPTR)"dos.library", 0, struct DosLibrary *, DOSBase);
