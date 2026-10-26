/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
*/

#ifndef GALLIUM_INTERN_H
#define GALLIUM_INTERN_H

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif

#ifndef DEVICES_TIMER_H
#   include <devices/timer.h>
#endif

#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

#ifndef GALLIUM_GALLIUM_H
#   include <gallium/gallium.h>
#endif

#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef PROTO_OOP_H
#   include <proto/oop.h>
#endif

#define GB(lb)  ((struct GalliumBase *)lb)

#ifndef HIDD_GRAPHICS_H
#   include <hidd/gfx.h>
#endif

/*
 * Method bases live in the library base. The HIDD_BM_* / HIDD_Display_*
 * inlines of the interface headers expand these where they are used, in a
 * scope with a GalliumBase (the per-object lookup the headers may define
 * is replaced).
 */
#undef HiddBitMapBase
#undef HiddDisplayBase
#define HiddBitMapBase       (GB(GalliumBase)->bmMethodBase)
#define HiddDisplayBase      (GB(GalliumBase)->displayMethodBase)

#ifndef HIDD_GALLIUM_H
#   include <hidd/gallium.h>
#endif

#include <aros/surfacetrack.h>

#ifndef P_SCREEN_H
#   include "pipe/p_screen.h"
#endif

#define GALLIUM_SFB_MAXBUFFERS  3

/* Minimum interval between hook driven surface recomputes (about a vblank) */
#define GALLIUM_SFB_HOOK_INTERVAL   8000

struct GalliumBase
{
    struct Library              galb_Lib;

    char                        *fallback;
    struct Library              *fallbackmodule;

    OOP_Class                   *basegallium;
    OOP_AttrBase                gfxAttrBase;
    OOP_AttrBase                galliumAttrBase;
    OOP_AttrBase                bmAttrBase;

    /* methods we use .. */
    OOP_MethodID                galliumMId_UpdateRect;
    OOP_MethodID                galliumMId_DisplayResource;
    OOP_MethodID                galliumMId_DisplayResourceRP;

    /* Scan-out framebuffers (3D scan-out) */
    OOP_AttrBase                displayAttrBase;
    OOP_AttrBase                pixfmtAttrBase;
    OOP_MethodID                bmMethodBase;
    OOP_MethodID                displayMethodBase;
    OOP_MethodID                galliumMId_ResourceFromBitMap;
    OOP_Class                   *basebm;         /* CLID_Hidd_BitMap, for CreateObject */

    struct SignalSemaphore      sfbLock;         /* the list; taken before any fb lock */
    struct MinList              sfbList;         /* attached GalliumScanoutFB          */
    struct SurfaceTrackNotify   sfbNotify;
    BOOL                        sfbNotifyActive;

    /* timer.device for the damage hook rate limit (ReadEClock only) */
    struct MsgPort              *timerMP;
    struct timerequest          *timerIO;
    struct SurfaceTrackCtx      stctx;
};

/*
 * One scan-out framebuffer: a multi-buffer scan-out bitmap of the window's
 * screen display, one pipe_resource per buffer, attached to the display as
 * a client surface and tracked with the surfacetrack helper.
 *
 * Lock order: GalliumBase->sfbLock -> fb->lock -> tracker lock -> layer
 * lock. The surface hook runs in a foreign task and only ever Attempts.
 */
struct GalliumScanoutFB
{
    struct MinNode                  node;       /* in GalliumBase->sfbList while attached */
    struct SignalSemaphore          lock;
    struct Library                  *base;

    OOP_Object                      *pipe;      /* the gallium hidd object        */
    APTR                            screen;     /* struct pipe_screen *           */
    OOP_Object                      *display;   /* the screen's display object    */
    OOP_Object                      *bm;        /* scan-out bitmap object         */
    struct Window                   *window;    /* NULL once detached             */
    struct Screen                   *wscreen;

    struct HIDD_CompositeRequest    req;
    struct Hook                     hook;       /* compositor -> us               */
    struct SurfaceTrack             track;
    struct Rectangle                inner;      /* layer relative rectangle       */

    ULONG                           width;
    ULONG                           height;
    ULONG                           buffers;
    ULONG                           back;       /* buffer to render next          */
    ULONG                           swaps;
    ULONG                           pipeformat; /* enum pipe_format               */
    HIDDT_StdPixFmt                 stdpf;
    struct pipe_resource            *res[GALLIUM_SFB_MAXBUFFERS];

    volatile BOOL                   onplane;    /* PlaneGained/PlaneLost          */
    BOOL                            attached;
    BOOL                            nowait;
};

/* scanoutframebuffer.c */
void gallium_sfb_CloseNotify(struct Window *window, APTR userData);

#undef HiddGfxAttrBase
#define HiddGfxAttrBase      (GB(GalliumBase)->gfxAttrBase)
#undef HiddBitMapAttrBase
#define HiddBitMapAttrBase      (GB(GalliumBase)->bmAttrBase)
#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase      (GB(GalliumBase)->galliumAttrBase)
#undef HiddDisplayAttrBase
#define HiddDisplayAttrBase      (GB(GalliumBase)->displayAttrBase)
#undef HiddPixFmtAttrBase
#define HiddPixFmtAttrBase       (GB(GalliumBase)->pixfmtAttrBase)

#endif
