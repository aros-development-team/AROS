/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Display backend of cgxvideo.library: the scan-out bitmap of a handle is
    attached to the screen's display as a client surface, tracked by the
    surfacetrack helper, and the colour key / backfill handling.
*/

#include <aros/debug.h>
#include <aros/asmcall.h>
#include <exec/memory.h>
#include <graphics/clip.h>
#include <graphics/gfxmacros.h>
#include <graphics/layers.h>
#include <cybergraphx/cybergraphics.h>
#include <cybergraphx/cgxvideo.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

/*****************************************************************************/

BOOL cgxv_GetDisplay(struct Screen *screen, OOP_Object **bmobjPtr, OOP_Object **displayPtr,
                     struct Library *CGXVideoBase)
{
    struct BitMap *bm;
    OOP_Object *bmobj;
    IPTR display = 0;

    if (!screen)
        return FALSE;

    bm = screen->RastPort.BitMap;
    if (!bm || !IS_HIDD_BM(bm))
        return FALSE;

    bmobj = HIDD_BM_OBJ(bm);
    if (!bmobj)
        return FALSE;

    OOP_GetAttr(bmobj, aHidd_BitMap_Display, &display);
    if (!display)
        return FALSE;

    *bmobjPtr   = bmobj;
    *displayPtr = (OOP_Object *)display;

    return TRUE;
}

BOOL cgxv_MapFormat(ULONG srctype, HIDDT_StdPixFmt *pixfmt, ULONG *videofmt)
{
    *pixfmt   = vHidd_StdPixFmt_Unknown;
    *videofmt = vHidd_VideoFmt_None;

    switch (srctype)
    {
    case SRCFMT_YUV16:
    case SRCFMT_YCbCr16:
        *videofmt = vHidd_VideoFmt_YUYV;
        return TRUE;

    case SRCFMT_RGB15PC:
        *pixfmt = vHidd_StdPixFmt_RGB15_LE;
        return TRUE;

    case SRCFMT_RGB16PC:
        *pixfmt = vHidd_StdPixFmt_RGB16_LE;
        return TRUE;

    case SRCFMT_YCbCr420:
        /* CGX planar 4:2:0 is I420: three planes, written in place */
        *videofmt = vHidd_VideoFmt_I420;
        return TRUE;

    case SRCFMT_YCbCr420_NV12:
        /* AROS extension: semi-planar, Y plane + interleaved UV plane */
        *videofmt = vHidd_VideoFmt_NV12;
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************/

static inline void cgxv_InnerRect(struct VLayerHandle *vh, struct Window *window,
                                  struct Rectangle *inner)
{
    st_WindowInnerRect(window, vh->indents[0], vh->indents[1],
                       vh->indents[2], vh->indents[3], inner);
}

/*
 * Copy the tracker's result into the request and tell the compositor.
 * vh->lock held.
 */
static void cgxv_PushGeometry(struct VLayerHandle *vh)
{
    struct Library *CGXVideoBase = vh->base;

    vh->req.Dest  = vh->track.Dest;
    vh->req.Shape = vh->track.Shape;

    if ((vh->req.Dest.MaxX - vh->req.Dest.MinX) != (vh->req.Src.MaxX - vh->req.Src.MinX) ||
        (vh->req.Dest.MaxY - vh->req.Dest.MinY) != (vh->req.Src.MaxY - vh->req.Src.MinY))
        vh->req.Flags |= CRF_SCALED;
    else
        vh->req.Flags &= ~CRF_SCALED;

    if ((vh->flags & VHF_ATTACHED) && !(vh->flags & VHF_NULL))
        HIDD_Display_UpdateSurface(vh->display, &vh->req);
}

/*
 * Deferred recompute, called first by every public entry point with
 * vh->lock held. May block on the layer lock (client task).
 */
void cgxv_Refresh(struct VLayerHandle *vh)
{
    struct Rectangle inner;

    if (!(vh->flags & VHF_ATTACHED) || (vh->flags & VHF_NULL) || !vh->window)
        return;

    if (!st_IsStale(&vh->track))
        return;

    cgxv_InnerRect(vh, vh->window, &inner);
    if (st_Compute(&vh->track, vh->window, &inner, NULL, NULL, TRUE) &&
        st_Changed(&vh->track))
        cgxv_PushGeometry(vh);
}

/*****************************************************************************/

/*
 * Compositor -> client surface hook. Runs in the damaging task with no
 * compositor lock held; it must neither block nor allocate heavily.
 */
AROS_UFH3(static IPTR, cgxv_SurfaceHook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, display, A2),
    AROS_UFHA(struct HIDD_SurfaceHookMsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct VLayerHandle *vh = hook->h_Data;
    struct Rectangle inner;

    if (!msg || msg->Type != vHidd_SurfaceHook_Damage)
        return 0;

    if (!AttemptSemaphore(&vh->lock))
    {
        st_SetStale(&vh->track);
        return 0;
    }

    if ((vh->flags & VHF_ATTACHED) && vh->window)
    {
        cgxv_InnerRect(vh, vh->window, &inner);
        if (st_HookDamageRect(&vh->track, msg, &inner))
        {
            st_Changed(&vh->track);
            cgxv_PushGeometry(vh);
        }
    }

    ReleaseSemaphore(&vh->lock);

    return 0;

    AROS_USERFUNC_EXIT
}

/*****************************************************************************/

static void cgxv_FillRect(struct VLayerHandle *vh, struct RastPort *rp, struct Rectangle *r)
{
    struct Library *CGXVideoBase = vh->base;
    ULONG oldpen = GetAPen(rp);
    ULONG olddrmd = GetDrMd(rp);

    (void)CGXVideoBase;

    if (vh->colorkeypen >= 0)
    {
        SetDrMd(rp, JAM1);
        SetAPen(rp, vh->colorkeypen);
        RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MaxY);
        SetAPen(rp, oldpen);
        SetDrMd(rp, olddrmd);
    }
    else
    {
        FillPixelArray(rp, r->MinX, r->MinY,
                       r->MaxX - r->MinX + 1, r->MaxY - r->MinY + 1, vh->colorkey);
    }
}

/* Fill the key into the window's surface rectangle. vh->lock held. */
static void cgxv_FillKey(struct VLayerHandle *vh)
{
    struct Rectangle inner;

    cgxv_InnerRect(vh, vh->window, &inner);
    /* RPort coordinates are WLayer relative, as the inner rectangle is */
    cgxv_FillRect(vh, vh->window->RPort, &inner);
}

/* Hand a part of a backfill message to the hook we replaced */
static void cgxv_ChainBackfill(struct VLayerHandle *vh, struct RastPort *rp,
                               struct BackFillMessage *msg, struct Rectangle *bmrect)
{
    struct Hook *old = vh->oldBackfill;
    struct BackFillMessage m;

    if (old == LAYERS_NOBACKFILL)
        return;

    if (old == LAYERS_BACKFILL)
    {
        /* the default: clear, as layers.library does */
        BltBitMap(rp->BitMap, 0, 0, rp->BitMap, bmrect->MinX, bmrect->MinY,
                  bmrect->MaxX - bmrect->MinX + 1, bmrect->MaxY - bmrect->MinY + 1,
                  0x000, 0xff, NULL);
        return;
    }

    m         = *msg;
    m.Bounds  = *bmrect;
    m.OffsetX = msg->OffsetX + (bmrect->MinX - msg->Bounds.MinX);
    m.OffsetY = msg->OffsetY + (bmrect->MinY - msg->Bounds.MinY);
    CallHookPkt(old, rp, &m);
}

/*
 * Layer backfill hook (VOA_UseBackfill): fills the key inside the surface
 * rectangle and chains to the previous hook outside it. Bounds are in the
 * coordinates of rp->BitMap, OffsetX/Y the layer relative position of
 * Bounds.MinX/MinY.
 */
AROS_UFH3(static void, cgxv_BackfillHook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct RastPort *, rp, A2),
    AROS_UFHA(struct BackFillMessage *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct VLayerHandle *vh = hook->h_Data;
    struct RastPort tmp;
    struct Rectangle key, is, part;
    LONG dx = msg->Bounds.MinX - msg->OffsetX;
    LONG dy = msg->Bounds.MinY - msg->OffsetY;

    /* The key rectangle in the message's bitmap coordinates */
    key.MinX = vh->track.Inner.MinX + dx;
    key.MinY = vh->track.Inner.MinY + dy;
    key.MaxX = vh->track.Inner.MaxX + dx;
    key.MaxY = vh->track.Inner.MaxY + dy;

    is.MinX = key.MinX > msg->Bounds.MinX ? key.MinX : msg->Bounds.MinX;
    is.MinY = key.MinY > msg->Bounds.MinY ? key.MinY : msg->Bounds.MinY;
    is.MaxX = key.MaxX < msg->Bounds.MaxX ? key.MaxX : msg->Bounds.MaxX;
    is.MaxY = key.MaxY < msg->Bounds.MaxY ? key.MaxY : msg->Bounds.MaxY;

    if (!(vh->flags & VHF_ATTACHED) || is.MinX > is.MaxX || is.MinY > is.MaxY)
    {
        cgxv_ChainBackfill(vh, rp, msg, &msg->Bounds);
        return;
    }

    /* Draw through a layerless copy: we are inside a layers operation */
    tmp = *rp;
    tmp.Layer = NULL;
    cgxv_FillRect(vh, &tmp, &is);

    /* Above */
    if (msg->Bounds.MinY < is.MinY)
    {
        part = msg->Bounds;
        part.MaxY = is.MinY - 1;
        cgxv_ChainBackfill(vh, rp, msg, &part);
    }
    /* Below */
    if (msg->Bounds.MaxY > is.MaxY)
    {
        part = msg->Bounds;
        part.MinY = is.MaxY + 1;
        cgxv_ChainBackfill(vh, rp, msg, &part);
    }
    /* Left */
    if (msg->Bounds.MinX < is.MinX)
    {
        part.MinX = msg->Bounds.MinX;
        part.MaxX = is.MinX - 1;
        part.MinY = is.MinY;
        part.MaxY = is.MaxY;
        cgxv_ChainBackfill(vh, rp, msg, &part);
    }
    /* Right */
    if (msg->Bounds.MaxX > is.MaxX)
    {
        part.MinX = is.MaxX + 1;
        part.MaxX = msg->Bounds.MaxX;
        part.MinY = is.MinY;
        part.MaxY = is.MaxY;
        cgxv_ChainBackfill(vh, rp, msg, &part);
    }

    AROS_USERFUNC_EXIT
}

/*****************************************************************************/

/*
 * Attach. CGXVideoBase->handleLock and vh->lock held; the handle is
 * neither locked nor attached. Returns 0 on success.
 */
ULONG cgxv_Attach(struct VLayerHandle *vh, struct Window *window, struct TagItem *tags)
{
    struct Library *CGXVideoBase = vh->base;
    struct IntCGXVBase *base = GetCGXVBase(CGXVideoBase);
    struct Rectangle inner, dest;
    struct Region *shape = NULL;
    OOP_Object *screenbm;

    vh->indents[0] = GetTagData(VOA_LeftIndent,   0, tags);
    vh->indents[1] = GetTagData(VOA_TopIndent,    0, tags);
    vh->indents[2] = GetTagData(VOA_RightIndent,  0, tags);
    vh->indents[3] = GetTagData(VOA_BottomIndent, 0, tags);

    if (!(vh->flags & VHF_NULL))
    {
        if (window->WScreen != vh->screen)
            return 1;

        cgxv_InnerRect(vh, window, &inner);
        if (!st_Compute(&vh->track, window, &inner, &dest, &shape, TRUE))
            return 1;
        st_Changed(&vh->track);

        vh->req.Next      = NULL;
        vh->req.BitMap    = vh->bm;
        vh->req.Src.MinX  = 0;
        vh->req.Src.MinY  = 0;
        vh->req.Src.MaxX  = vh->width - 1;
        vh->req.Src.MaxY  = vh->height - 1;
        vh->req.Dest      = dest;
        vh->req.Shape     = shape;
        vh->req.Opacity   = 255;
        vh->req.AlphaMode = vHidd_CompAlpha_None;
        vh->req.Flags     = CRF_PLANE_REQUIRED;
        vh->req.ZOrder    = 0;
        vh->req.UserData  = NULL;
        if ((dest.MaxX - dest.MinX) != (LONG)(vh->width - 1) ||
            (dest.MaxY - dest.MinY) != (LONG)(vh->height - 1))
            vh->req.Flags |= CRF_SCALED;

        vh->hook.h_Entry    = (HOOKFUNC)AROS_ASMSYMNAME(cgxv_SurfaceHook);
        vh->hook.h_SubEntry = NULL;
        vh->hook.h_Data     = vh;

        screenbm = HIDD_BM_OBJ(vh->screen->RastPort.BitMap);
        if (!HIDD_Display_AttachSurface(vh->display, screenbm, &vh->req, &vh->hook))
        {
            D(bug("[CGXVideo] %s: AttachSurface refused\n", __func__));
            st_Reset(&vh->track);
            vh->req.Shape = NULL;
            return 1;
        }
    }

    vh->window = window;
    vh->flags |= VHF_ATTACHED;
    AddTail((struct List *)&base->handles, (struct Node *)&vh->node);

    if ((vh->flags & (VHF_COLORKEY | VHF_NULL)) == VHF_COLORKEY)
    {
        cgxv_FillKey(vh);

        if (vh->flags & VHF_BACKFILL)
        {
            vh->backfillHook.h_Entry    = (HOOKFUNC)AROS_ASMSYMNAME(cgxv_BackfillHook);
            vh->backfillHook.h_SubEntry = NULL;
            vh->backfillHook.h_Data     = vh;
            vh->oldBackfill = InstallLayerHook(window->WLayer, &vh->backfillHook);
            vh->flags |= VHF_BACKFILLHOOK;
        }
    }

    return 0;
}

/* Detach. handleLock and vh->lock held. Safe to call when not attached. */
void cgxv_Detach(struct VLayerHandle *vh)
{
    struct Library *CGXVideoBase = vh->base;

    if (!(vh->flags & VHF_ATTACHED))
        return;

    if (vh->flags & VHF_BACKFILLHOOK)
    {
        InstallLayerHook(vh->window->WLayer, vh->oldBackfill);
        vh->oldBackfill = NULL;
        vh->flags &= ~VHF_BACKFILLHOOK;
    }

    if (!(vh->flags & VHF_NULL))
        HIDD_Display_DetachSurface(vh->display, &vh->req);

    Remove((struct Node *)&vh->node);
    vh->window = NULL;
    vh->flags &= ~VHF_ATTACHED;
    st_Reset(&vh->track);
    vh->req.Shape = NULL;
}

/* End a buffer lock. vh->lock held. */
void cgxv_UnlockBuffer(struct VLayerHandle *vh)
{
    struct Library *CGXVideoBase = vh->base;

    if (!vh->locked)
        return;

    if (!(vh->flags & VHF_NULL))
        HIDD_BM_UnlockBuffer(vh->bm, vh->lockedBuffer, NULL);

    vh->locked     = FALSE;
    vh->lockedAddr = NULL;
}

/*
 * SNOTIFY_BEFORE_CLOSEWINDOW safety net: detach whatever is still attached
 * to the closing window. Runs in the closing task.
 */
void cgxv_CloseNotify(struct Window *window, APTR userData)
{
    struct IntCGXVBase *base = userData;
    struct VLayerHandle *vh, *next;

    ObtainSemaphore(&base->handleLock);
    ForeachNodeSafe(&base->handles, vh, next)
    {
        if (vh->window == window)
        {
            ObtainSemaphore(&vh->lock);
            D(bug("[CGXVideo] %s: window %p closing with handle %p attached\n", __func__, window, vh));
            cgxv_UnlockBuffer(vh);
            cgxv_Detach(vh);
            ReleaseSemaphore(&vh->lock);
        }
    }
    ReleaseSemaphore(&base->handleLock);
}
