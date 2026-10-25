/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Scan-out framebuffers (3D scan-out, VIDEO-OVERLAY-3D-SCANOUT-PLAN 3.7):
    a multi-buffer scan-out bitmap of the window's screen display, aliased
    into the pipe as one pipe_resource per buffer, attached to the display
    as a client surface (CRF_PLANE_OK: a plane when the compositor grants
    one, blitted through the visible region otherwise) and tracked with the
    surfacetrack helper exactly as cgxvideo.library tracks a video layer.

    Lock order: GalliumBase->sfbLock -> fb->lock -> tracker lock -> layer
    lock. The surface hook runs in the damaging task and only Attempts.
*/

#include <aros/debug.h>

/* aros/debug.h leaves DEBUG defined (0 when unset); Mesa's u_debug_refcnt.h
 * tests defined(DEBUG) and would then reference debug_reference_slowpath. */
#undef DEBUG

#include <aros/asmcall.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "gallium_intern.h"

/*****************************************************************************/

static BOOL sfb_MapFormat(ULONG pipeformat, HIDDT_StdPixFmt *stdpf)
{
    switch (pipeformat)
    {
    case PIPE_FORMAT_B8G8R8A8_UNORM:
        *stdpf = vHidd_StdPixFmt_BGRA32;
        return TRUE;
    case PIPE_FORMAT_B8G8R8X8_UNORM:
        *stdpf = vHidd_StdPixFmt_BGR032;
        return TRUE;
    case PIPE_FORMAT_B5G6R5_UNORM:
        *stdpf = vHidd_StdPixFmt_RGB16_LE;
        return TRUE;
    }
    return FALSE;
}

/*
 * Copy the tracker's result into the request and tell the compositor.
 * fb->lock held.
 */
static void sfb_PushGeometry(struct GalliumScanoutFB *fb)
{
    struct Library *GalliumBase = fb->base;

    fb->req.Dest  = fb->track.Dest;
    fb->req.Shape = fb->track.Shape;

    if (fb->attached)
        HIDD_Display_UpdateSurface(fb->display, &fb->req);
}

/*
 * Deferred recompute (the hook could not take a lock, or rate limited).
 * fb->lock held; may block on the layer lock (client task).
 */
static void sfb_Refresh(struct GalliumScanoutFB *fb)
{
    if (!fb->attached || !fb->window)
        return;

    if (!st_IsStale(&fb->track))
        return;

    if (st_Compute(&fb->track, fb->window, &fb->inner, NULL, NULL, TRUE) &&
        st_Changed(&fb->track))
        sfb_PushGeometry(fb);
}

/*
 * Compositor -> client surface hook. Runs in the damaging task with no
 * compositor lock held; it must neither block nor allocate heavily.
 */
AROS_UFH3(static IPTR, sfb_SurfaceHook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, display, A2),
    AROS_UFHA(struct HIDD_SurfaceHookMsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct GalliumScanoutFB *fb = hook->h_Data;

    if (!msg)
        return 0;

    switch (msg->Type)
    {
    case vHidd_SurfaceHook_PlaneGained:
        fb->onplane = TRUE;
        D(bug("[Gallium] sfb %p: on plane %lu\n", fb, (unsigned long)msg->Plane));
        return 0;

    case vHidd_SurfaceHook_PlaneLost:
        fb->onplane = FALSE;
        D(bug("[Gallium] sfb %p: plane lost\n", fb));
        return 0;

    case vHidd_SurfaceHook_Damage:
        break;

    default:
        return 0;
    }

    if (!AttemptSemaphore(&fb->lock))
    {
        st_SetStale(&fb->track);
        return 0;
    }

    if (fb->attached && fb->window)
    {
        if (st_HookDamageRect(&fb->track, msg, &fb->inner))
        {
            st_Changed(&fb->track);
            sfb_PushGeometry(fb);
        }
    }

    ReleaseSemaphore(&fb->lock);

    return 0;

    AROS_USERFUNC_EXIT
}

/* Free everything of a framebuffer that is not (or no longer) attached */
static void sfb_Free(struct GalliumScanoutFB *fb)
{
    ULONG i;

    for (i = 0; i < GALLIUM_SFB_MAXBUFFERS; i++)
        pipe_resource_reference(&fb->res[i], NULL);

    if (fb->bm)
        OOP_DisposeObject(fb->bm);

    st_Cleanup(&fb->track);
    FreeVec(fb);
}

/* Detach from the display and the list. sfbLock and fb->lock held. */
static void sfb_Detach(struct GalliumScanoutFB *fb)
{
    struct Library *GalliumBase = fb->base;

    if (!fb->attached)
        return;

    HIDD_Display_DetachSurface(fb->display, &fb->req);
    Remove((struct Node *)&fb->node);
    fb->attached = FALSE;
    fb->onplane  = FALSE;
    fb->window   = NULL;
    st_Reset(&fb->track);
    fb->req.Shape = NULL;
}

/*
 * SNOTIFY_BEFORE_CLOSEWINDOW safety net: detach whatever is still attached
 * to the closing window. Runs in the closing task. The framebuffer itself
 * stays valid (its resources too) until the owner destroys it.
 */
void gallium_sfb_CloseNotify(struct Window *window, APTR userData)
{
    struct Library *GalliumBase = userData;
    struct GalliumScanoutFB *fb, *next;

    ObtainSemaphore(&GB(GalliumBase)->sfbLock);
    ForeachNodeSafe(&GB(GalliumBase)->sfbList, fb, next)
    {
        if (fb->window == window)
        {
            ObtainSemaphore(&fb->lock);
            D(bug("[Gallium] %s: window %p closing with framebuffer %p attached\n", __func__, window, fb));
            sfb_Detach(fb);
            ReleaseSemaphore(&fb->lock);
        }
    }
    ReleaseSemaphore(&GB(GalliumBase)->sfbLock);
}

/*****************************************************************************

    NAME */

      AROS_LH3(APTR, CreateScanoutFrameBuffer,

/*  SYNOPSIS */
      AROS_LHA(PipeHandle_t, pipe, A0),
      AROS_LHA(struct Window *, window, A1),
      AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
      struct Library *, GalliumBase, 10, Gallium)

/*  FUNCTION
        Creates a scan-out framebuffer for a rectangle of a window: 2 or 3
        colour buffers in display memory that the window's screen display
        shows itself (through a hardware plane when the compositor grants
        one, blitted through the window's visible region otherwise), each
        available to the pipe as a pipe_resource to render into. The
        framebuffer follows the window: gallium.library tracks the window's
        layer and keeps the surface's position and visible shape current.

    INPUTS
        pipe   - the pipe handle from CreatePipe()
        window - the window; its screen's display shows the framebuffer
        tags   - SFB_Left, SFB_Top, SFB_Width, SFB_Height (required: the
                 rectangle relative to window->WLayer, i.e. RastPort
                 coordinates), SFB_PipeFormat (required), SFB_PipeScreen
                 (required: the PipeScreen_t of this pipe), SFB_Buffers
                 (2 or 3, default 2), SFB_NoWait (see <gallium/gallium.h>)

    RESULT
        The framebuffer handle, or NULL when the display cannot show the
        format through a plane (HIDD_Display_QueryScanout without
        SCF_PLANE), the pipe driver cannot import scan-out buffers
        (moHidd_Gallium_ResourceFromBitMap), or the surface cannot be
        attached (e.g. the screen is not shown yet). The caller then keeps
        BltPipeResourceRastPort().

    NOTES
        Rendering goes to the buffer SwapScanoutFrameBuffer() returned (the
        SFBA_BackBuffer at creation); the pipe_resource of buffer n is
        GetScanoutFrameBufferResource(fb, n). There is no resize: destroy
        and create a new framebuffer when the rectangle changes.

        The framebuffer must be destroyed before the window closes; as a
        safety net gallium.library detaches it when the window closes
        (SNOTIFY_BEFORE_CLOSEWINDOW), after which swaps show nothing and
        SFBA_Attached is FALSE.

    SEE ALSO
        GetScanoutFrameBufferResource(), SwapScanoutFrameBuffer(),
        DestroyScanoutFrameBuffer(), GetScanoutFrameBufferAttr()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GalliumScanoutFB *fb;
    struct HIDD_ScanoutCaps caps;
    struct BitMap *screenbitmap;
    OOP_Object *screenbm;
    IPTR display = 0, val = 0;
    LONG left, top, width, height;
    ULONG buffers, pipeformat, i;
    APTR screen;
    HIDDT_StdPixFmt stdpf;
    struct Rectangle dest;
    struct Region *shape = NULL;

    if (!pipe || !window || !tags || !window->WScreen)
        return NULL;

    left       = GetTagData(SFB_Left,       0, tags);
    top        = GetTagData(SFB_Top,        0, tags);
    width      = GetTagData(SFB_Width,      0, tags);
    height     = GetTagData(SFB_Height,     0, tags);
    buffers    = GetTagData(SFB_Buffers,    2, tags);
    pipeformat = GetTagData(SFB_PipeFormat, PIPE_FORMAT_NONE, tags);
    screen     = (APTR)GetTagData(SFB_PipeScreen, 0, tags);

    if (width <= 0 || height <= 0 || !screen)
        return NULL;
    if (!sfb_MapFormat(pipeformat, &stdpf))
    {
        D(bug("[Gallium] %s: pipe format %lu not mapped\n", __func__, (unsigned long)pipeformat));
        return NULL;
    }
    if (buffers < 2)
        buffers = 2;
    if (buffers > GALLIUM_SFB_MAXBUFFERS)
        buffers = GALLIUM_SFB_MAXBUFFERS;

    screenbitmap = window->WScreen->RastPort.BitMap;
    if (!screenbitmap || !IS_HIDD_BM(screenbitmap))
        return NULL;
    screenbm = HIDD_BM_OBJ(screenbitmap);
    if (!screenbm)
        return NULL;

    OOP_GetAttr(screenbm, aHidd_BitMap_Display, &display);
    if (!display)
        return NULL;

    memset(&caps, 0, sizeof(caps));
    if (!(HIDD_Display_QueryScanout((OOP_Object *)display, stdpf, 0, width, height, &caps) & SCF_PLANE))
    {
        D(bug("[Gallium] %s: no plane for std pixfmt %lu %ldx%ld on screen %p\n", __func__,
              (unsigned long)stdpf, (long)width, (long)height, window->WScreen));
        return NULL;
    }
    if (caps.MaxBuffers && buffers > caps.MaxBuffers)
        buffers = caps.MaxBuffers;
    if (buffers < 2)
    {
        D(bug("[Gallium] %s: display allows %lu buffer(s) only\n", __func__, (unsigned long)caps.MaxBuffers));
        return NULL;
    }

    fb = AllocVec(sizeof(*fb), MEMF_PUBLIC | MEMF_CLEAR);
    if (!fb)
        return NULL;

    InitSemaphore(&fb->lock);
    fb->base       = GalliumBase;
    fb->pipe       = (OOP_Object *)pipe;
    fb->screen     = screen;
    fb->display    = (OOP_Object *)display;
    fb->wscreen    = window->WScreen;
    fb->width      = width;
    fb->height     = height;
    fb->pipeformat = pipeformat;
    fb->stdpf      = stdpf;
    fb->inner.MinX = left;
    fb->inner.MinY = top;
    fb->inner.MaxX = left + width - 1;
    fb->inner.MaxY = top + height - 1;

    if (!st_Init(&fb->track, &GB(GalliumBase)->stctx))
    {
        FreeVec(fb);
        return NULL;
    }

    {
        struct TagItem bmtags[] =
        {
            { aHidd_BitMap_Scanout,   TRUE            },
            { aHidd_BitMap_Width,     width           },
            { aHidd_BitMap_Height,    height          },
            { aHidd_BitMap_Buffers,   buffers         },
            { aHidd_BitMap_StdPixFmt, (IPTR)stdpf     },
            { aHidd_BitMap_Friend,    (IPTR)screenbm  },
            { TAG_DONE,               0               }
        };

        fb->bm = HIDD_Display_CreateObject((OOP_Object *)display, GB(GalliumBase)->basebm, bmtags);
    }

    if (!fb->bm)
    {
        D(bug("[Gallium] %s: scan-out bitmap %ldx%ld x%lu refused\n", __func__,
              (long)width, (long)height, (unsigned long)buffers));
        sfb_Free(fb);
        return NULL;
    }

    val = 0;
    OOP_GetAttr(fb->bm, aHidd_BitMap_Buffers, &val);
    fb->buffers = val;
    if (fb->buffers < 2 || fb->buffers > GALLIUM_SFB_MAXBUFFERS)
    {
        D(bug("[Gallium] %s: bitmap has %lu buffer(s)\n", __func__, (unsigned long)fb->buffers));
        sfb_Free(fb);
        return NULL;
    }
    fb->nowait = GetTagData(SFB_NoWait, (fb->buffers >= 3) ? TRUE : FALSE, tags) ? TRUE : FALSE;

    /* One pipe_resource per buffer, aliasing the display memory */
    for (i = 0; i < fb->buffers; i++)
    {
        struct pHidd_Gallium_ResourceFromBitMap rfbmsg =
        {
            .mID    = GB(GalliumBase)->galliumMId_ResourceFromBitMap,
            .bitmap = fb->bm,
            .buffer = i,
            .screen = screen,
        };

        fb->res[i] = (struct pipe_resource *)OOP_DoMethod((OOP_Object *)pipe, (OOP_Msg)&rfbmsg);
        if (!fb->res[i])
        {
            D(bug("[Gallium] %s: driver cannot import buffer %lu\n", __func__, (unsigned long)i));
            sfb_Free(fb);
            return NULL;
        }
    }

    /* Attach the surface */
    if (!st_Compute(&fb->track, window, &fb->inner, &dest, &shape, TRUE))
    {
        sfb_Free(fb);
        return NULL;
    }
    st_Changed(&fb->track);

    fb->req.Next      = NULL;
    fb->req.BitMap    = fb->bm;
    fb->req.Src.MinX  = 0;
    fb->req.Src.MinY  = 0;
    fb->req.Src.MaxX  = width - 1;
    fb->req.Src.MaxY  = height - 1;
    fb->req.Dest      = dest;
    fb->req.Shape     = shape;
    fb->req.Opacity   = 255;
    fb->req.AlphaMode = vHidd_CompAlpha_None;
    fb->req.Flags     = CRF_PLANE_OK;
    fb->req.ZOrder    = 0;
    fb->req.UserData  = NULL;

    fb->hook.h_Entry    = (HOOKFUNC)AROS_ASMSYMNAME(sfb_SurfaceHook);
    fb->hook.h_SubEntry = NULL;
    fb->hook.h_Data     = fb;

    ObtainSemaphore(&GB(GalliumBase)->sfbLock);
    if (!HIDD_Display_AttachSurface((OOP_Object *)display, screenbm, &fb->req, &fb->hook))
    {
        ReleaseSemaphore(&GB(GalliumBase)->sfbLock);
        D(bug("[Gallium] %s: AttachSurface refused\n", __func__));
        st_Reset(&fb->track);
        fb->req.Shape = NULL;
        sfb_Free(fb);
        return NULL;
    }
    fb->window   = window;
    fb->attached = TRUE;
    AddTail((struct List *)&GB(GalliumBase)->sfbList, (struct Node *)&fb->node);
    ReleaseSemaphore(&GB(GalliumBase)->sfbLock);

    val = 0;
    OOP_GetAttr(fb->bm, aHidd_BitMap_BackBuffer, &val);
    fb->back = val;

    D(bug("[Gallium] %s: fb %p window %p %ldx%ld at %ld,%ld, %lu buffers, back %lu, %s\n", __func__,
          fb, window, (long)width, (long)height, (long)left, (long)top,
          (unsigned long)fb->buffers, (unsigned long)fb->back, fb->nowait ? "no wait" : "vblank wait"));

    return fb;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

      AROS_LH2(APTR, GetScanoutFrameBufferResource,

/*  SYNOPSIS */
      AROS_LHA(APTR, fb, A0),
      AROS_LHA(ULONG, buffer, D0),

/*  LOCATION */
      struct Library *, GalliumBase, 11, Gallium)

/*  FUNCTION
        Returns the pipe_resource that aliases colour buffer `buffer` of a
        scan-out framebuffer.

    INPUTS
        fb     - the framebuffer
        buffer - 0 .. SFBA_Buffers - 1

    RESULT
        struct pipe_resource *, owned by the framebuffer (take your own
        reference with pipe_resource_reference() if you keep it), or NULL.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GalliumScanoutFB *sfb = fb;

    if (!sfb || buffer >= sfb->buffers)
        return NULL;

    return sfb->res[buffer];

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, SwapScanoutFrameBuffer,

/*  SYNOPSIS */
      AROS_LHA(APTR, fb, A0),

/*  LOCATION */
      struct Library *, GalliumBase, 12, Gallium)

/*  FUNCTION
        Shows the buffer that was rendered last (the one the previous call,
        or SFBA_BackBuffer at creation, handed out) and returns the index
        of the buffer to render next. The caller must have flushed its
        rendering to the pipe before the call. With 2 buffers the call
        waits until the displaced buffer is off scan-out (unless
        SFB_NoWait); with 3 it returns at once and the returned buffer is
        one that is not scanned out.

    INPUTS
        fb - the framebuffer

    RESULT
        Index of the buffer to render next.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GalliumScanoutFB *sfb = fb;
    IPTR back = 0;

    if (!sfb)
        return 0;

    ObtainSemaphore(&sfb->lock);

    /* A recompute the hook had to defer */
    sfb_Refresh(sfb);

    HIDD_BM_SwapBuffers(sfb->bm, sfb->back, sfb->nowait ? fHidd_Swap_NoWait : 0);

    /* Never hand out a buffer that is still scanned out: the driver's
     * BackBuffer is the one it has released. */
    OOP_GetAttr(sfb->bm, aHidd_BitMap_BackBuffer, &back);
    if (back >= sfb->buffers)
        back = 0;
    sfb->back = back;
    sfb->swaps++;

    ReleaseSemaphore(&sfb->lock);

    return back;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

      AROS_LH1(void, DestroyScanoutFrameBuffer,

/*  SYNOPSIS */
      AROS_LHA(APTR, fb, A0),

/*  LOCATION */
      struct Library *, GalliumBase, 13, Gallium)

/*  FUNCTION
        Detaches the surface from the display and frees the framebuffer and
        its resources. Resources the caller still references stay valid
        until its last reference is dropped, but their memory is no longer
        shown.

    INPUTS
        fb - the framebuffer (NULL is ignored)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GalliumScanoutFB *sfb = fb;

    if (!sfb)
        return;

    ObtainSemaphore(&GB(GalliumBase)->sfbLock);
    ObtainSemaphore(&sfb->lock);
    sfb_Detach(sfb);
    ReleaseSemaphore(&sfb->lock);
    ReleaseSemaphore(&GB(GalliumBase)->sfbLock);

    D(bug("[Gallium] %s: fb %p after %lu swaps\n", __func__, sfb, (unsigned long)sfb->swaps));

    sfb_Free(sfb);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

      AROS_LH2(IPTR, GetScanoutFrameBufferAttr,

/*  SYNOPSIS */
      AROS_LHA(APTR, fb, A0),
      AROS_LHA(ULONG, attr, D0),

/*  LOCATION */
      struct Library *, GalliumBase, 14, Gallium)

/*  FUNCTION
        Queries a scan-out framebuffer.

    INPUTS
        fb   - the framebuffer
        attr - SFBA_* (see <gallium/gallium.h>)

    RESULT
        The value, 0 for an unknown attribute or a NULL framebuffer.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GalliumScanoutFB *sfb = fb;
    IPTR val = 0;

    if (!sfb)
        return 0;

    switch (attr)
    {
    case SFBA_Buffers:    return sfb->buffers;
    case SFBA_Width:      return sfb->width;
    case SFBA_Height:     return sfb->height;
    case SFBA_OnPlane:    return sfb->onplane ? TRUE : FALSE;
    case SFBA_PipeFormat: return sfb->pipeformat;
    }

    ObtainSemaphore(&sfb->lock);
    switch (attr)
    {
    case SFBA_BackBuffer:
        val = sfb->back;
        break;
    case SFBA_FrontBuffer:
        OOP_GetAttr(sfb->bm, aHidd_BitMap_FrontBuffer, &val);
        break;
    case SFBA_Attached:
        val = sfb->attached ? TRUE : FALSE;
        break;
    case SFBA_Window:
        val = (IPTR)sfb->window;
        break;
    case SFBA_Swaps:
        val = sfb->swaps;
        break;
    }
    ReleaseSemaphore(&sfb->lock);

    return val;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

      AROS_LH1(APTR, FindScanoutFrameBuffer,

/*  SYNOPSIS */
      AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
      struct Library *, GalliumBase, 15, Gallium)

/*  FUNCTION
        Returns the scan-out framebuffer currently attached to a window, if
        any (the first one in attach order). Meant for diagnostics (a GL
        program asking whether its context presents through scan-out); the
        handle belongs to whoever created it and is only valid while that
        owner keeps it.

    INPUTS
        window - the window

    RESULT
        The framebuffer handle or NULL.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GalliumScanoutFB *sfb, *found = NULL;

    if (!window)
        return NULL;

    ObtainSemaphore(&GB(GalliumBase)->sfbLock);
    ForeachNode(&GB(GalliumBase)->sfbList, sfb)
    {
        if (sfb->window == window)
        {
            found = sfb;
            break;
        }
    }
    ReleaseSemaphore(&GB(GalliumBase)->sfbLock);

    return found;

    AROS_LIBFUNC_EXIT
}
