#ifndef CGXVIDEO_INTERN_H
#define CGXVIDEO_INTERN_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <devices/timer.h>
#include <dos/bptr.h>
#include <aros/libcall.h>
#include <graphics/rastport.h>
#include <graphics/view.h>
#include <graphics/layers.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <intuition/intuition.h>
#include <oop/oop.h>
#include <cybergraphx/cgxvideo.h>

struct IntCGXVBase;

/*
 * Method and attribute bases live in the library base. The method base
 * macros must be defined before <hidd/gfx.h> pulls in the interface
 * headers (they only provide a per-object lookup when undefined); the
 * attribute base macros are redefined after it.
 */
#define GetCGXVBase(base)   ((struct IntCGXVBase *)(base))
#define HiddBitMapBase      (GetCGXVBase(CGXVideoBase)->cgxv_HiddBitMapBase)
#define HiddDisplayBase     (GetCGXVBase(CGXVideoBase)->cgxv_HiddDisplayBase)

#include <hidd/gfx.h>

#undef HiddBitMapAttrBase
#undef HiddDisplayAttrBase
#define HiddBitMapAttrBase  (GetCGXVBase(CGXVideoBase)->cgxv_HiddBitMapAttrBase)
#define HiddDisplayAttrBase (GetCGXVBase(CGXVideoBase)->cgxv_HiddDisplayAttrBase)

#include <aros/surfacetrack.h>

struct IntCGXVBase
{
    struct Library            libnode;

    OOP_AttrBase              cgxv_HiddBitMapAttrBase;
    OOP_AttrBase              cgxv_HiddDisplayAttrBase;
    struct OOP_ABDescr        attrbases[3];
    OOP_MethodID              cgxv_HiddBitMapBase;
    OOP_MethodID              cgxv_HiddDisplayBase;
    OOP_Class                *basebm;             /* CLID_Hidd_BitMap, for CreateObject */

    /* Attached handles (for the close-window safety net) */
    struct SignalSemaphore    handleLock;
    struct MinList            handles;
    struct SurfaceTrackNotify notify;
    BOOL                      notifyActive;

    /* timer.device for the damage hook rate limit */
    struct MsgPort           *timerMP;
    struct timerequest       *timerIO;
    struct SurfaceTrackCtx    stctx;
};

/* VLayerHandle flags */
#define VHF_NULL        (1 << 0)    /* null backend: source in system RAM, nothing shown */
#define VHF_COLORKEY    (1 << 1)    /* VOA_UseColorKey                                   */
#define VHF_BACKFILL    (1 << 2)    /* VOA_UseBackfill                                   */
#define VHF_ATTACHED    (1 << 3)    /* surface attached to a window                      */
#define VHF_BACKFILLHOOK (1 << 4)   /* our backfill hook is installed                    */

struct VLayerHandle
{
    struct MinNode               node;      /* in CGXVideoBase->handles while attached */
    struct SignalSemaphore       lock;      /* guards the state below                  */
    struct Library              *base;

    struct Screen               *screen;
    OOP_Object                  *display;   /* the screen's display object             */
    OOP_Object                  *bm;        /* scan-out bitmap object                  */
    struct Window               *window;    /* NULL while detached                     */

    struct HIDD_CompositeRequest req;
    struct Hook                  hook;      /* surface hook (compositor -> us)         */
    struct SurfaceTrack          track;

    struct Hook                  backfillHook;
    struct Hook                 *oldBackfill;

    ULONG                        flags;     /* VHF_*                                   */
    ULONG                        srctype;   /* SRCFMT_*                                */
    ULONG                        videofmt;  /* vHidd_VideoFmt_* or 0                   */
    HIDDT_StdPixFmt              pixfmt;    /* RGB sources                             */
    ULONG                        width;
    ULONG                        height;
    ULONG                        modulo;    /* bytes per row (null backend / fallback) */
    ULONG                        buffers;
    LONG                         indents[4];/* left, top, right, bottom                */

    BOOL                         locked;
    ULONG                        lockedBuffer;
    UBYTE                       *lockedAddr;
    ULONG                        lockedPitch;

    ULONG                        colorkey;  /* 0x00RRGGBB                              */
    LONG                         colorkeypen; /* -1 = none                             */

    ULONG                        planeOffsets[2]; /* I420: U and V plane offsets from the
                                                      Y plane, from the last LockBuffer */

    APTR                         nullbuffer; /* null backend source data               */
};

/* 4:2:0 sources, written in place: I420 (three planes) or NV12 (Y + interleaved UV) */
#define VH_NV12(vh)     ((vh)->srctype == SRCFMT_YCbCr420_NV12)
#define VH_PLANAR(vh)   ((vh)->srctype == SRCFMT_YCbCr420 || VH_NV12(vh))

/* nullbackend.c */
BOOL cgxv_NullBackend(void);

/* backend.c */
BOOL  cgxv_GetDisplay(struct Screen *screen, OOP_Object **bmobjPtr, OOP_Object **displayPtr,
                      struct Library *CGXVideoBase);
BOOL  cgxv_MapFormat(ULONG srctype, HIDDT_StdPixFmt *pixfmt, ULONG *videofmt);
void  cgxv_Refresh(struct VLayerHandle *vh);
ULONG cgxv_Attach(struct VLayerHandle *vh, struct Window *window, struct TagItem *tags);
void  cgxv_Detach(struct VLayerHandle *vh);
void  cgxv_UnlockBuffer(struct VLayerHandle *vh);
void  cgxv_CloseNotify(struct Window *window, APTR userData);

#endif /* CGXVIDEO_INTERN_H */
