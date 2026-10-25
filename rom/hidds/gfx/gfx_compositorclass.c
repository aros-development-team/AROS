/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.

    Desc: Base display compositor class.

    Every Display object owns one compositor (see Display::New). The
    compositor decides what the display scans out when the display driver
    does not compose in hardware: it keeps the Z-ordered stack of screen
    bitmaps handed over by ShowViewPorts(), computes the visible region of
    each, blits those regions into a display bitmap (the driver's framebuffer
    where there is one), blends COMPF_ALPHA screens over an intermediate
    bitmap, and shows the frontmost bitmap directly (passthrough) whenever it
    covers the whole display.

    This class was the "Compositor" monitor driver (Devs/Monitors/Compositor)
    before composition became a service of the Display. It no longer talks to
    Intuition: per-screen composition flags and the pre-alpha hook arrive in
    the HIDD_ViewPortData chain, filled in by graphics.library.
*/

#define DEBUG 0
#if (DEBUG)
#define DTOGGLE(x) x
#define DMOVE(x) x
#define DRECALC(x) x
#define DREDRAWBM(x) x
#define DREDRAWSCR(x) x
#define DSTACK(x) x
#define DUPDATE(x) x
#else
#define DTOGGLE(x)
#define DMOVE(x)
#define DRECALC(x)
#define DREDRAWBM(x)
#define DREDRAWSCR(x)
#define DSTACK(x)
#define DUPDATE(x)
#endif

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <graphics/view.h>
#include <hidd/gfx.h>
#include <string.h>

#include "gfx_intern.h"
#include "gfx_compositor.h"

#include LC_LIBDEFS_FILE

/*
 * The helpers below take the instance data, not the class: resolve the
 * module static data (and with it the method/attribute bases) through it.
 */
#undef csd
#define csd (compdata->compcsd)

#undef GfxBase
#define GfxBase (compdata->GraphicsBase)
#undef DOSBase
#define DOSBase (compdata->dosbase)
#undef UtilityBase
#define UtilityBase (csd->cs_UtilityBase)
#undef OOPBase
#define OOPBase (csd->cs_OOPBase)

#define COMPOSITOR_PREFS        "SYS/compositor.prefs"
#define COMPOSITOR_PEFSTEMPLATE "ABOVE/S,BELOW/S,LEFT/S,RIGHT/S,ALPHA/S,SCALE/S,NATIVEMODE/S,NOPLANES/S"

/* What the software path can honour */
#define CAPABILITY_FLAGS (COMPF_ABOVE|COMPF_BELOW|COMPF_LEFT|COMPF_RIGHT|COMPF_ALPHA|COMPF_SCALE)

enum
{
    ARG_ABOVE = 0,
    ARG_BELOW,
    ARG_LEFT,
    ARG_RIGHT,
    ARG_ALPHA,
    ARG_SCALE,
    ARG_NATIVEMODE,
    ARG_NOPLANES,
    NOOFARGS
};

#define _RECT(x) x.MinX, x.MinY, x.MaxX, x.MaxY

#define LOCK_COMPOSITOR_READ       { ObtainSemaphoreShared(&compdata->semaphore); }
#define LOCK_COMPOSITOR_WRITE      { ObtainSemaphore(&compdata->semaphore); }
#define UNLOCK_COMPOSITOR          { ReleaseSemaphore(&compdata->semaphore); }

/* Hooks installed with AROS_ASMSYMNAME() use the register-passing convention */
static inline IPTR comp_CallHook(struct Hook *h, APTR object, APTR message)
{
    return AROS_UFC3(IPTR, h->h_Entry,
                     AROS_UFCA(struct Hook *, h,       A0),
                     AROS_UFCA(APTR,          object,  A2),
                     AROS_UFCA(APTR,          message, A1));
}

/****************************************************************************************/

static BOOL isRectInRegion(struct Region *region, struct Rectangle *rect)
{
    struct RegionRectangle *rrect = region->RegionRectangle;

    while (rrect)
    {
        struct Rectangle r =
        {
            rrect->bounds.MinX + region->bounds.MinX,
            rrect->bounds.MinY + region->bounds.MinY,
            rrect->bounds.MaxX + region->bounds.MinX,
            rrect->bounds.MaxY + region->bounds.MinY
        };

        if ((r.MinX <= rect->MaxX) && (r.MaxX >= rect->MinX) &&
            (r.MinY <= rect->MaxY) && (r.MaxY >= rect->MinY))
            return TRUE;

        rrect = rrect->Next;
    }
    return FALSE;
}

/*
 * graphics.library is initialised after gfx.hidd, so it cannot be opened when
 * the compositor is created. Every entry point that needs it (regions, bitmap
 * allocation) runs from LoadView() or later, when it is up.
 */
static BOOL comp_EnsureGraphics(struct HIDDCompositorData *compdata)
{
    if (!compdata->GraphicsBase)
        compdata->GraphicsBase = (APTR)OpenLibrary("graphics.library", 41);

    return compdata->GraphicsBase != NULL;
}

/*
 * ENV:SYS/compositor.prefs holds the enabled composition types. It can only
 * be read once DOS and ENV: exist, and only from a Process; it is tried on
 * every stack change until it has been read. Until then the AmigaOS-like
 * default (ABOVE) applies.
 */
static void comp_ReadPrefs(struct HIDDCompositorData *compdata)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct RDArgs *rdargs;
    IPTR CompArgs[NOOFARGS] = { 0 };
    TEXT CompConfig[256];
    APTR old_windowptr;
    LONG len;

    if (compdata->prefsread || (me->pr_Task.tc_Node.ln_Type != NT_PROCESS))
        return;

    if (!compdata->dosbase)
        compdata->dosbase = OpenLibrary("dos.library", 0);
    if (!compdata->dosbase)
        return;

    /* No "Please insert volume ENV:" requester: this may run while a screen opens */
    old_windowptr = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;

    rdargs = AllocDosObjectTags(DOS_RDARGS, TAG_END);
    if (rdargs)
    {
        len = GetVar(COMPOSITOR_PREFS, CompConfig, sizeof(CompConfig), GVF_GLOBAL_ONLY);
        if (len != -1)
        {
            ULONG caps = 0;

            rdargs->RDA_Source.CS_Buffer = CompConfig;
            rdargs->RDA_Source.CS_Length = len;
            rdargs->RDA_DAList = (IPTR)NULL;
            rdargs->RDA_Buffer = NULL;
            rdargs->RDA_BufSiz = 0;
            rdargs->RDA_ExtHelp = NULL;
            rdargs->RDA_Flags = 0;

            if (ReadArgs(COMPOSITOR_PEFSTEMPLATE, CompArgs, rdargs))
            {
                if (CompArgs[ARG_ABOVE]) caps |= COMPF_ABOVE;
                if (CompArgs[ARG_BELOW]) caps |= COMPF_BELOW;
                if (CompArgs[ARG_LEFT])  caps |= COMPF_LEFT;
                if (CompArgs[ARG_RIGHT]) caps |= COMPF_RIGHT;
                if (CompArgs[ARG_ALPHA]) caps |= COMPF_ALPHA;
                /*
                 * SCALE: the display takes the mode of the largest screen shown and
                 * the smaller screens are scaled up to it. NATIVEMODE: the display
                 * keeps its own highest mode and every screen is scaled (user option).
                 */
                /* NOPLANES: compose everything, never use the display's scan-out planes */
                compdata->noplanes = CompArgs[ARG_NOPLANES] ? TRUE : FALSE;
                if (CompArgs[ARG_NATIVEMODE])
                    compdata->modepolicy = vHidd_Compositor_ModePolicy_Native;
                else if (CompArgs[ARG_SCALE])
                    compdata->modepolicy = vHidd_Compositor_ModePolicy_ScaleLargest;
                else
                    compdata->modepolicy = vHidd_Compositor_ModePolicy_FollowTop;
                FreeArgs(rdargs);

                compdata->capabilities = caps;
                D(bug("[Compositor] %s: prefs state %08lx\n", __func__, caps));
            }
            compdata->prefsread = TRUE;
        }
        else if (IoErr() == ERROR_OBJECT_NOT_FOUND)
        {
            /* ENV: exists, the variable does not: keep the default */
            compdata->prefsread = TRUE;
        }
        FreeDosObject(DOS_RDARGS, rdargs);
    }

    me->pr_WindowPtr = old_windowptr;
}

/****************************************************************************************/
/* Surface hook calls                                                                   */
/****************************************************************************************/

/*
 * Hook firing rule. A client surface's hook (damage, plane gained/lost,
 * screen moved) is called with no compositor lock held, from the task whose
 * call caused the event. Calls are collected into a HookFires list while the
 * lock is held (QueueHook counts the pending call in ClientSurface.firing
 * under hooksem) and made once the lock is released (FireHooks: the call,
 * then the counter goes down under hooksem and a DetachSurface waiting for
 * zero is woken). DetachSurface unlinks the surface under the write lock, so
 * no further call can be collected for it, releases the lock, and waits
 * (SIGF_SINGLE) until every call collected before that has returned; only
 * then is the entry freed and the method returns. Hence the hook is never
 * entered after DetachSurface returns, and the entry it is given is never
 * freed under a running hook. For the client: UpdateSurface may be called
 * from the hook (no lock is held), DetachSurface must not (it would wait for
 * the hook call it is made from).
 */
#define HOOKFIRE_CHUNK 8

struct HookFire
{
    struct ClientSurface       *cs;
    struct HIDD_SurfaceHookMsg  msg;
};

struct HookFires
{
    struct HookFires *next;     /* overflow chunks, allocated on demand */
    ULONG             count;
    struct HookFire   f[HOOKFIRE_CHUNK];
};

static inline void HIDDCompositorInitFires(struct HookFires *hf)
{
    hf->next  = NULL;
    hf->count = 0;
}

/* Lock held: remember a hook call for HIDDCompositorFireHooks() */
static void HIDDCompositorQueueHook(struct HIDDCompositorData *compdata, struct HookFires *hf, struct ClientSurface *cs,
                                    ULONG type, struct Rectangle *rect, ULONG plane)
{
    struct HookFire *f;

    if (!hf || !cs->hook)
        return;
    while (hf->count == HOOKFIRE_CHUNK)
    {
        if (!hf->next)
        {
            hf->next = AllocMem(sizeof(struct HookFires), MEMF_ANY);
            if (!hf->next)
                return;     /* out of memory: this call is lost */
            HIDDCompositorInitFires(hf->next);
        }
        hf = hf->next;
    }
    f = &hf->f[hf->count++];
    f->cs          = cs;
    f->msg.Type    = type;
    f->msg.Request = cs->req;
    f->msg.Plane   = plane;
    if (rect)
        f->msg.Rect = *rect;
    else
        f->msg.Rect.MinX = f->msg.Rect.MinY = f->msg.Rect.MaxX = f->msg.Rect.MaxY = 0;

    ObtainSemaphore(&compdata->hooksem);
    cs->firing++;
    ReleaseSemaphore(&compdata->hooksem);
}

/* Lock released: make the collected calls, free the overflow chunks */
static void HIDDCompositorFireHooks(struct HIDDCompositorData *compdata, struct HookFires *hf)
{
    struct HookFires *chunk = hf;

    while (chunk)
    {
        struct HookFires *next = chunk->next;
        ULONG i;

        for (i = 0; i < chunk->count; i++)
        {
            struct HookFire *f = &chunk->f[i];

            CallHookPkt(f->cs->hook, compdata->display, &f->msg);

            ObtainSemaphore(&compdata->hooksem);
            if ((--f->cs->firing == 0) && f->cs->waiter)
                Signal(f->cs->waiter, SIGF_SINGLE);
            ReleaseSemaphore(&compdata->hooksem);
        }
        if (chunk != hf)
            FreeMem(chunk, sizeof(struct HookFires));
        chunk = next;
    }
}

static void HIDDCompositorRecalculateSurfaces(struct HIDDCompositorData *compdata, struct StackBitMapNode *n);
static VOID HIDDCompositorRecalculateVisibleRegions(struct HIDDCompositorData *compdata);
static VOID HIDDCompositorRedrawVisibleRegions(struct HIDDCompositorData *compdata, struct Rectangle *drawrect);
static void HIDDCompositorQueryPlanes(struct HIDDCompositorData *compdata);
static void HIDDCompositorAssignPlanes(struct HIDDCompositorData *compdata);
static void HIDDCompositorPlanPlanes(struct HIDDCompositorData *compdata);
static void HIDDCompositorCommitPlanes(struct HIDDCompositorData *compdata, ULONG flags, struct HookFires *fires);
static void HIDDCompositorQueuePlaneHooks(struct HIDDCompositorData *compdata, struct HookFires *fires);
static BOOL HIDDCompositorToggleCompositing(struct HIDDCompositorData *compdata, BOOL newtop);
static struct StackBitMapNode *HIDDCompositorFindBitMapStackNode(struct HIDDCompositorData *compdata, OOP_Object *bm);

/****************************************************************************************/
/* Screen geometry on the display                                                       */
/****************************************************************************************/

/*
 * Screen pixels and display pixels differ when the mode policy is Native: the
 * display keeps its own mode and every screen is scaled to fill it. All the
 * geometry below goes through these helpers; with FollowTop they are identities.
 */
static inline LONG scr2disp(LONG v, LONG scr, LONG dst)
{
    return (scr == dst) ? v : (LONG)(((QUAD)v * dst) / scr);
}

static inline LONG disp2scr(LONG v, LONG scr, LONG dst)
{
    return (scr == dst) ? v : (LONG)(((QUAD)v * scr) / dst);
}

#define NODE_X2D(n, x) scr2disp((x), (n)->scrw, (n)->dstw)
#define NODE_Y2D(n, y) scr2disp((y), (n)->scrh, (n)->dsth)
#define NODE_X2S(n, x) disp2scr((x), (n)->scrw, (n)->dstw)
#define NODE_Y2S(n, y) disp2scr((y), (n)->scrh, (n)->dsth)

/* The screen's rectangle on the display */
static inline void HIDDCompositorNodeRect(struct StackBitMapNode *n, struct Rectangle *r)
{
    r->MinX = NODE_X2D(n, n->leftedge);
    r->MinY = NODE_Y2D(n, n->topedge);
    r->MaxX = r->MinX + n->dstw - 1;
    r->MaxY = r->MinY + n->dsth - 1;
}

/* A screen relative rectangle (inclusive, screen pixels) on the display */
static inline void HIDDCompositorNodeMapRect(struct StackBitMapNode *n, LONG x0, LONG y0, LONG x1, LONG y1, struct Rectangle *r)
{
    r->MinX = NODE_X2D(n, n->leftedge + x0);
    r->MinY = NODE_Y2D(n, n->topedge + y0);
    r->MaxX = NODE_X2D(n, n->leftedge + x1 + 1) - 1;
    r->MaxY = NODE_Y2D(n, n->topedge + y1 + 1) - 1;
}

/* Fill in each node's screen and display size for the current mode and policy */
static void HIDDCompositorUpdateScaling(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode *n;
    LONG dispw = compdata->displayrect.MaxX - compdata->displayrect.MinX + 1;
    LONG disph = compdata->displayrect.MaxY - compdata->displayrect.MinY + 1;

    ForeachNode(&compdata->bitmapstack, n)
    {
        IPTR w = 0, h = 0;

        OOP_GetAttr(n->bm, aHidd_BitMap_Width, &w);
        OOP_GetAttr(n->bm, aHidd_BitMap_Height, &h);
        n->scrw = w;
        n->scrh = h;
        if ((compdata->modepolicy != vHidd_Compositor_ModePolicy_FollowTop) && w && h && dispw > 0 && disph > 0)
        {
            n->dstw = dispw;
            n->dsth = disph;
        }
        else
        {
            n->dstw = w;
            n->dsth = h;
        }
        n->scaled = (n->dstw != n->scrw) || (n->dsth != n->scrh);
        if (n->scaled)
            n->sbmflags |= COMPF_SCALE;
    }
}

static BOOL HIDDCompositorScaleBuffers(struct HIDDCompositorData *compdata, ULONG srclen, ULONG dstlen)
{
    if (srclen > compdata->scalesrclen)
    {
        if (compdata->scalesrc)
            FreeVec(compdata->scalesrc);
        compdata->scalesrc = AllocVec(srclen * sizeof(ULONG), MEMF_ANY);
        compdata->scalesrclen = compdata->scalesrc ? srclen : 0;
    }
    if (dstlen > compdata->scaledstlen)
    {
        if (compdata->scaledst)
            FreeVec(compdata->scaledst);
        if (compdata->scalemap)
            FreeVec(compdata->scalemap);
        compdata->scaledst = AllocVec(dstlen * sizeof(ULONG), MEMF_ANY);
        compdata->scalemap = AllocVec(dstlen * sizeof(UWORD), MEMF_ANY);
        compdata->scaledstlen = (compdata->scaledst && compdata->scalemap) ? dstlen : 0;
    }
    return compdata->scalesrc && compdata->scaledst && compdata->scalemap;
}

/*
 * Draw 'rect' (display relative) of a bitmap shown scaled: its pixels from
 * srcx0,srcy0 (srcw x srch) map onto the display rectangle at orgx,orgy
 * (dstw x dsth). Nearest neighbour, one row at a time through the driver as
 * ARGB32, so any pair of pixel formats works and no direct access is needed;
 * every destination pixel is mapped exactly, so adjacent damage rectangles
 * never seam. 'alpha' blends (source over) with the source alpha scaled by
 * 'opacity' instead of copying.
 */
static void HIDDCompositorScaledBlit(struct HIDDCompositorData *compdata, OOP_Object *target, OOP_Object *src,
                                     LONG srcx0, LONG srcy0, LONG srcw, LONG srch,
                                     LONG orgx, LONG orgy, LONG dstw, LONG dsth,
                                     struct Rectangle *rect, UBYTE opacity, BOOL alpha)
{
    LONG w = rect->MaxX - rect->MinX + 1;
    LONG sxfirst = disp2scr(rect->MinX - orgx, srcw, dstw);
    LONG sxlast  = disp2scr(rect->MaxX - orgx, srcw, dstw);
    LONG sw = sxlast - sxfirst + 1;
    LONG lastsy = -1;
    LONG x, y;
    ULONG *srcbuf, *dstbuf;
    UWORD *map;

    if ((w <= 0) || (sw <= 0) || !dstw || !dsth)
        return;
    if (sxfirst < 0 || sxlast >= srcw)
        return;
    if (!HIDDCompositorScaleBuffers(compdata, sw, w))
        return;
    srcbuf = compdata->scalesrc;
    dstbuf = compdata->scaledst;
    map    = compdata->scalemap;

    for (x = 0; x < w; x++)
        map[x] = disp2scr(rect->MinX + x - orgx, srcw, dstw) - sxfirst;

    for (y = rect->MinY; y <= rect->MaxY; y++)
    {
        LONG sy = disp2scr(y - orgy, srch, dsth);

        if (sy < 0 || sy >= srch)
            continue;
        if (sy != lastsy)
        {
            HIDD_BM_GetImage(src, (UBYTE *)srcbuf, sw * sizeof(ULONG), srcx0 + sxfirst, srcy0 + sy, sw, 1, vHidd_StdPixFmt_ARGB32);
            for (x = 0; x < w; x++)
                dstbuf[x] = srcbuf[map[x]];
            if (alpha && (opacity != 0xFF))
            {
                /* ARGB32 is byte order: the alpha byte comes first */
                UBYTE *a = (UBYTE *)dstbuf;
                for (x = 0; x < w; x++, a += 4)
                    *a = (UBYTE)(((ULONG)*a * opacity + 127) / 255);
            }
            lastsy = sy;
        }
        if (alpha)
            HIDD_BM_PutAlphaImage(target, compdata->gc, (UBYTE *)dstbuf, w * sizeof(ULONG), rect->MinX, y, w, 1);
        else
            HIDD_BM_PutImage(target, compdata->gc, (UBYTE *)dstbuf, w * sizeof(ULONG), rect->MinX, y, w, 1, vHidd_StdPixFmt_ARGB32);
    }
}

/****************************************************************************************/
/* Display mode selection                                                               */
/****************************************************************************************/

/* ModePolicy_Native: the display's own mode is taken to be its largest truecolor one */
static void HIDDCompositorFindNativeMode(struct HIDDCompositorData *compdata)
{
    HIDDT_ModeID mode = vHidd_ModeID_Invalid;
    OOP_Object *sync, *pf;
    ULONG best = 0;

    if (compdata->nativeknown)
        return;
    compdata->nativeknown = TRUE;

    while ((mode = HIDD_DMEnum_NextModeID(compdata->dmenum, mode, &sync, &pf)) != vHidd_ModeID_Invalid)
    {
        IPTR w = 0, h = 0;

        if (OOP_GET(pf, aHidd_PixFmt_ColorModel) != vHidd_ColorModel_TrueColor)
            continue;
        OOP_GetAttr(sync, aHidd_Sync_HDisp, &w);
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &h);
        if ((ULONG)(w * h) > best)
        {
            best = w * h;
            compdata->nativew = w;
            compdata->nativeh = h;
        }
    }
}

static HIDDT_ModeID FindBestHiddMode(struct HIDDCompositorData *compdata, ULONG width, ULONG height, ULONG depth, ULONG *res_depth)
{
    HIDDT_ModeID mode = vHidd_ModeID_Invalid;
    OOP_Object *sync, *pf;
    IPTR w, h, d;
    ULONG dw, dh, delta;
    ULONG found_delta  = -1;
    ULONG found_width  = 0;
    ULONG found_height = 0;
    ULONG found_depth  = 0;
    HIDDT_ModeID found_mode = vHidd_ModeID_Invalid;

    D(bug("[Compositor] %s: Finding best match for mode %ux%ux%u\n", __func__, width, height, depth));

    while ((mode = HIDD_DMEnum_NextModeID(compdata->dmenum, mode, &sync, &pf)) != vHidd_ModeID_Invalid)
    {
        BOOL match;

        if (OOP_GET(pf, aHidd_PixFmt_ColorModel) != vHidd_ColorModel_TrueColor)
            continue;

        OOP_GetAttr(sync, aHidd_Sync_HDisp, &w);
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &h);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &d);

        dw = w > width  ? w - width  : w < width  ? width  - w : 1;
        dh = h > height ? h - height : h < height ? height - h : 1;
        delta = dw * dh;

        match = FALSE;
        if (delta < found_delta)
        {
            /* If mode resolution is closer to the needed one, we've got a better match */
            found_delta  = delta;
            found_width  = w;
            found_height = h;

            match = TRUE;
        }
        else if (delta == found_delta)
        {
            /* If resolution is the same as that of current candidate mode, we can look at depth. */
            if (found_depth > depth)
            {
                /*
                 * Candidate mode if deeper than requested. We can supersede it with another mode
                 * of smaller depth, but which still matches our request.
                 */
                if ((d < found_depth) && (d >= depth))
                    match = TRUE;
            }
            else if (found_depth < depth)
            {
                /*
                 * We want better depth than current candidate.
                 * In this case anything deeper will do.
                 */
                if (d > found_depth)
                    match = TRUE;
            }
        }

        if (match)
        {
            found_depth = d;
            found_mode  = mode;
        }
    }

    /* Store mode information */
    compdata->displayrect.MinX = 0;
    compdata->displayrect.MinY = 0;
    compdata->displayrect.MaxX = found_width  - 1;
    compdata->displayrect.MaxY = found_height - 1;
    *res_depth = found_depth;

    return found_mode;
}

/* Follows the resolution of biggest screen */
static void CalculateParametersAlpha(struct HIDDCompositorData *compdata, ULONG *wantedwidth,
        ULONG *wantedheight, ULONG *wanteddepth)
{
    struct StackBitMapNode  *n;
    OOP_Object              *sync, *pf;
    IPTR                    curwidth, curheight, curdepth, modeid;

    for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
             n->n.mln_Pred; n = (struct StackBitMapNode *)n->n.mln_Pred)
    {
        if ((!(n->sbmflags & COMPF_ALPHA)) && (n->sbmflags & STACKNODEF_DISPLAYABLE))
        {
            OOP_GetAttr(n->bm, aHidd_BitMap_ModeID, &modeid);
            HIDD_DMEnum_GetMode(compdata->dmenum, modeid, &sync, &pf);

            if (sync)
            {
                /* Get the needed size */
                OOP_GetAttr(sync, aHidd_Sync_HDisp, &curwidth);
                OOP_GetAttr(sync, aHidd_Sync_VDisp, &curheight);
            }
            else
            {
                OOP_GetAttr(n->bm, aHidd_BitMap_Width, &curwidth);
                OOP_GetAttr(n->bm, aHidd_BitMap_Height, &curheight);
            }

            if (OOP_GET(pf, aHidd_PixFmt_ColorModel) == vHidd_ColorModel_TrueColor)
            {
                OOP_GetAttr(pf, aHidd_PixFmt_Depth, &curdepth);
            }
            else
            {
                /* If we have a LUT bitmap on stack, we request atleast 16-bit screen.
                   if DEEPLUT is set be use 24bit instead (for better color matching) */
                if (compdata->flags & COMPSTATEF_DEEPLUT)
                    curdepth = 24;
                else
                    curdepth = 16;
            }
        }
        else
        {
            OOP_GetAttr(n->bm, aHidd_BitMap_Width, &curwidth);
            OOP_GetAttr(n->bm, aHidd_BitMap_Height, &curheight);

            if (n->sbmflags & COMPF_ALPHA)
                curdepth = 24;
            else
            {
                OOP_GetAttr(n->bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);
                if (OOP_GET(pf, aHidd_PixFmt_ColorModel) == vHidd_ColorModel_TrueColor)
                {
                    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &curdepth);
                }
                else
                {
                    if (compdata->flags & COMPSTATEF_DEEPLUT)
                        curdepth = 24;
                    else
                        curdepth = 16;
                }
            }
        }
        if ((ULONG)curwidth > *wantedwidth)
            *wantedwidth = (ULONG)curwidth;
        if ((ULONG)curheight > *wantedheight)
            *wantedheight = (ULONG)curheight;
        if ((ULONG)curdepth > *wanteddepth)
            *wanteddepth = (ULONG)curdepth;
    }
}

/* Follows the resolution of topmost screen */
static void CalculateParametersRegular(struct HIDDCompositorData *compdata, ULONG *wantedwidth,
        ULONG *wantedheight, ULONG *wanteddepth)
{
    struct StackBitMapNode  *n;
    OOP_Object              *sync = NULL, *pf;
    IPTR                    width, height, depth, modeid;

    /*
     * Examine all bitmaps in the stack to figure out the needed depth.
     * We need a maximum depth of all depths in order to keep correct colors.
     * But not less than 16 bits, because we can't compose on a LUT screen.
     *
     * If a LUT bitmap is present in the stack (depth < 9), we request truecolor
     * screen for better color presentation.
     *
     * We examine bitmaps in reverse order, in this case 'sync' will hold
     * information about the top bitmap when we exit the loop.
     * Size of our composited mode needs to be as close as possible to that one.
     */
    for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
         n->n.mln_Pred; n = (struct StackBitMapNode *)n->n.mln_Pred)
    {
        OOP_GetAttr(n->bm, aHidd_BitMap_ModeID, &modeid);
        HIDD_DMEnum_GetMode(compdata->dmenum, modeid, &sync, &pf);

        if (OOP_GET(pf, aHidd_PixFmt_ColorModel) == vHidd_ColorModel_TrueColor)
        {
            OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
            if ((ULONG)depth > *wanteddepth)
                *wanteddepth = (ULONG)depth;
        }
        else
        {
            /*
             * If we have a LUT bitmap on stack, we request 24-bit screen
             * for better color transfer.
             */
            *wanteddepth = 24;
        }
    }

    /* Get the needed size */
    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

    *wantedwidth   = (ULONG)width;
    *wantedheight  = (ULONG)height;
}

static void UpdateDisplayMode(struct HIDDCompositorData *compdata)
{
    ULONG                   wantedwidth = 0, wantedheight = 0, wanteddepth = 16;
    ULONG                   found_depth;
    IPTR                    modeid;

    D(bug("[Compositor] %s()\n", __func__));

    if ((compdata->flags & COMPSTATEF_HASALPHA) || (compdata->modepolicy == vHidd_Compositor_ModePolicy_ScaleLargest))
        CalculateParametersAlpha(compdata, &wantedwidth, &wantedheight, &wanteddepth);   /* the largest screen's mode */
    else
        CalculateParametersRegular(compdata, &wantedwidth, &wantedheight, &wanteddepth);

    if (compdata->modepolicy == vHidd_Compositor_ModePolicy_Native)
    {
        /* The display keeps its own mode; the stack only decides the depth */
        HIDDCompositorFindNativeMode(compdata);
        if (compdata->nativew && compdata->nativeh)
        {
            wantedwidth  = compdata->nativew;
            wantedheight = compdata->nativeh;
        }
    }

    D(bug("[Compositor] %s: Prefered mode %ldx%ldx%d\n", __func__, wantedwidth, wantedheight, wanteddepth));

    modeid = FindBestHiddMode(compdata, wantedwidth, wantedheight, wanteddepth, &found_depth);
    D(bug("[Compositor] %s: Composition Display ModeID 0x%08X [current 0x%08X]\n", __func__, modeid, compdata->displaymode));

    if (modeid != compdata->displaymode)
    {
        /* The mode is different. Need to prepare information needed for compositing */
        struct TagItem gctags[] =
        {
            { aHidd_GC_Foreground, 0x99999999 },
            { TAG_DONE           , 0          }
        };

        /* Signal mode change */
        compdata->displaymode = modeid;
        compdata->displaydepth = wanteddepth;
        compdata->modeschanged = TRUE;

        /* Get gray foregound */
        if (found_depth < 24)
            gctags[0].ti_Data = 0x9492;

        OOP_SetAttrs(compdata->gc, gctags);
    }
}

/****************************************************************************************/
/* Stack and region bookkeeping                                                         */
/****************************************************************************************/

static struct StackBitMapNode *HIDDCompositorFindBitMapStackNode(struct HIDDCompositorData *compdata, OOP_Object *bm)
{
    struct StackBitMapNode *n = NULL;

    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->bm == bm)
            return n;
    }

    return NULL;
}

static VOID HIDDCompositorValidatePosition(IPTR width, IPTR height,
                                           SIPTR *newxoffset, SIPTR *newyoffset,
                                           LONG displayedwidth, LONG displayedheight)
{
    LONG neglimit, poslimit;

    /* Check x position */
    if (width > displayedwidth)
    {
        neglimit = displayedwidth - width;
        poslimit = 0;
    }
    else
    {
        neglimit = 0;
        poslimit = displayedwidth - width;
    }

    if (*(newxoffset) > poslimit)
        *(newxoffset) = poslimit;
    if (*(newxoffset) < neglimit)
        *(newxoffset) = neglimit;

    /* Check y position */
    if (height > displayedheight)
        neglimit = displayedheight - height; /* Limit for scroll */
    else
        neglimit = 0;
    poslimit = displayedheight - 15; /* Limit for drag */

    if (*(newyoffset) > poslimit)
        *(newyoffset) = poslimit;
    if (*(newyoffset) < neglimit)
        *(newyoffset) = neglimit;
}

static VOID HIDDCompositorValidateBitMapPositionChange(struct HIDDCompositorData *compdata, OOP_Object *bm,
                                                       SIPTR *newxoffset, SIPTR *newyoffset,
                                                       LONG displayedwidth, LONG displayedheight)
{
    IPTR width, height;

    OOP_GetAttr(bm, aHidd_BitMap_Width, &width);
    OOP_GetAttr(bm, aHidd_BitMap_Height, &height);
    HIDDCompositorValidatePosition(width, height, newxoffset, newyoffset, displayedwidth, displayedheight);
}

/*
 * The same for a screen on display. A scaled screen is validated in display
 * pixels with its scaled size; the offsets (screen pixels) are only rewritten
 * when the clamp changed something, so rounding never creeps.
 */
static VOID HIDDCompositorValidateNodePositionChange(struct HIDDCompositorData *compdata, struct StackBitMapNode *n,
                                                     SIPTR *newxoffset, SIPTR *newyoffset,
                                                     LONG displayedwidth, LONG displayedheight)
{
    SIPTR dx, dy, cx, cy;

    if (!n->scaled)
    {
        HIDDCompositorValidatePosition(n->scrw, n->scrh, newxoffset, newyoffset, displayedwidth, displayedheight);
        return;
    }
    dx = cx = NODE_X2D(n, *newxoffset);
    dy = cy = NODE_Y2D(n, *newyoffset);
    HIDDCompositorValidatePosition(n->dstw, n->dsth, &cx, &cy, displayedwidth, displayedheight);
    if (cx != dx)
        *newxoffset = NODE_X2S(n, cx);
    if (cy != dy)
        *newyoffset = NODE_Y2S(n, cy);
}

static VOID HIDDCompositorRecalculateVisibleRegions(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode      *n = NULL, *tmpn;
    struct Region               *dispvisregion = NULL;
    ULONG                       oldflags = compdata->flags;

    DRECALC(bug("[Compositor] %s: Display rect [%d, %d - %d, %d]\n", __func__, _RECT(compdata->displayrect)));

    /*
     * This function assumes bitmapstack is in correct Z order:
     * from topmost to bottom most
     */
    if ((dispvisregion = NewRegion()) != NULL)
    {
        OrRectRegion(dispvisregion, &compdata->displayrect);

        compdata->flags &= ~COMPSTATEF_HASALPHA;
        if (compdata->alpharegion)
        {
            DisposeRegion(compdata->alpharegion);
            compdata->alpharegion = NULL;
        }

        ForeachNodeSafe(&compdata->bitmapstack, n, tmpn)
        {
            /* Get bitmap bounding box in screen coordinates */
            struct Rectangle tmprect, noderect;

            n->sbmflags &= ~STACKNODEF_VISIBLE;

            /* Scanned out by a plane above the composed bitmap: not part of it */
            if (n->promoted)
            {
                if (n->screenregion)
                    ClearRegion(n->screenregion);
                /* Its client surfaces are placed by where the screen is on the display */
                if (n->hassurfaces)
                    HIDDCompositorRecalculateSurfaces(compdata, n);
                continue;
            }

            if (n->screenregion)
                ClearRegion(n->screenregion);
            else
                n->screenregion = NewRegion();

            if (n->screenregion)
            {
                struct RegionRectangle *srrect;

                HIDDCompositorNodeRect(n, &noderect);
                tmprect = noderect;

                DRECALC(bug("[Compositor] %s: Screen Rect [%d, %d - %d, %d]\n", __func__, _RECT(tmprect)));

                OrRectRegion(n->screenregion, &tmprect); // Start with the Screen's dimensions ..
                AndRegionRegion(dispvisregion, n->screenregion); // And adjust for the "Display"

                if ((srrect = n->screenregion->RegionRectangle) != NULL)
                {
                    while (srrect)
                    {
                        tmprect.MinX = srrect->bounds.MinX + n->screenregion->bounds.MinX;
                        tmprect.MinY = srrect->bounds.MinY + n->screenregion->bounds.MinY;
                        tmprect.MaxX = srrect->bounds.MaxX + n->screenregion->bounds.MinX;
                        tmprect.MaxY = srrect->bounds.MaxY + n->screenregion->bounds.MinY;

                        if (!(n->sbmflags & COMPF_ALPHA))
                            ClearRectRegion(dispvisregion, &tmprect);
                        else
                        {
                            compdata->flags |= COMPSTATEF_HASALPHA;
                            if (!(compdata->alpharegion))
                                compdata->alpharegion = NewRegion();

                            if (compdata->alpharegion)
                                OrRectRegion(compdata->alpharegion, &tmprect);
                        }
                        srrect = srrect->Next;
                    }

                    if (!(compdata->capabilities & COMPF_ABOVE) && !(n->sbmflags & COMPF_ABOVE))
                    {
                        tmprect.MinX = compdata->displayrect.MinX;
                        tmprect.MaxX = compdata->displayrect.MaxX;
                        tmprect.MinY = compdata->displayrect.MinY;
                        tmprect.MaxY = noderect.MinY - 1;
                        ClearRectRegion(dispvisregion, &tmprect);
                    }
                    if (!(compdata->capabilities & COMPF_BELOW) && !(n->sbmflags & COMPF_BELOW))
                    {
                        tmprect.MinX = compdata->displayrect.MinX;
                        tmprect.MaxX = compdata->displayrect.MaxX;
                        tmprect.MinY = noderect.MaxY + 1;
                        tmprect.MaxY = compdata->displayrect.MaxY;
                        ClearRectRegion(dispvisregion, &tmprect);
                    }
                    if (!(compdata->capabilities & COMPF_LEFT) && !(n->sbmflags & COMPF_LEFT))
                    {
                        tmprect.MinX = compdata->displayrect.MinX;
                        tmprect.MaxX = noderect.MinX - 1;
                        tmprect.MinY = noderect.MinY;
                        tmprect.MaxY = noderect.MaxY;
                        ClearRectRegion(dispvisregion, &tmprect);
                    }
                    if (!(compdata->capabilities & COMPF_RIGHT) && !(n->sbmflags & COMPF_RIGHT))
                    {
                        tmprect.MinX = noderect.MaxX + 1;
                        tmprect.MaxX = compdata->displayrect.MaxX;
                        tmprect.MinY = noderect.MinY;
                        tmprect.MaxY = noderect.MaxY;
                        ClearRectRegion(dispvisregion, &tmprect);
                    }
                    n->sbmflags |= STACKNODEF_VISIBLE;
                }

                if (n->hassurfaces)
                    HIDDCompositorRecalculateSurfaces(compdata, n);

                DRECALC(bug("[Compositor] %s: HiddBitmap 0x%p, topedge %d, visible %d\n", __func__,
                            n->bm, n->topedge, (n->sbmflags & STACKNODEF_VISIBLE)));
            }
            else
            {
                DRECALC(bug("[Compositor] %s: Failed to create Screen Region\n", __func__));
            }
        }
        DisposeRegion(dispvisregion);
        if (compdata->flags != oldflags)
        {
            ULONG newflags = (~oldflags) & compdata->flags;
            DRECALC(bug("[Compositor] %s: Newly set flags %08x\n", __func__, newflags));
            if ((!(newflags & COMPSTATEF_HASALPHA)) && (oldflags & COMPSTATEF_HASALPHA))
            {
                DRECALC(bug("[Compositor] %s: Alpha removed\n", __func__));
                if (compdata->alpharegion)
                    DisposeRegion(compdata->alpharegion);

                compdata->alpharegion = NULL;
            }
        }
    }
    else
    {
        DRECALC(bug("[Compositor] %s: Failed to create Display Region\n", __func__));
    }
}

/****************************************************************************************/
/* Scan-out planes                                                                      */
/****************************************************************************************/

/* Ask the display once what it can scan out directly */
static void HIDDCompositorQueryPlanes(struct HIDDCompositorData *compdata)
{
    IPTR count = 0;
    ULONG i;

    if (compdata->planesknown)
        return;
    compdata->planesknown = TRUE;

    if (compdata->noplanes)
        return;     /* prefs: planes switched off */
    OOP_GetAttr(compdata->display, aHidd_Display_PlaneCount, &count);
    if (count < 2)
        return;     /* only the primary: everything is composed into it, as before */
    if (count > 32)
        count = 32; /* the assignment tracks the planes in use in a 32 bit mask */

    compdata->planes = AllocMem(count * (sizeof(struct HIDD_PlaneInfo) + 2 * sizeof(struct HIDD_PlaneState)), MEMF_ANY | MEMF_CLEAR);
    if (!compdata->planes)
        return;
    compdata->planealloc = count;
    compdata->planestates = (struct HIDD_PlaneState *)&compdata->planes[count];
    compdata->commitstates = &compdata->planestates[count];
    {
        IPTR key = HIDD_DISPLAY_DEFAULT_COLORKEY;

        OOP_GetAttr(compdata->display, aHidd_Display_ColorKey, &key);
        compdata->colorkey = key;
    }

    for (i = 0; i < count; i++)
    {
        if (!HIDD_Display_GetPlaneInfo(compdata->display, i, &compdata->planes[i], sizeof(struct HIDD_PlaneInfo)))
            break;
        compdata->planestates[i].Plane = i;
    }
    compdata->planecount = i;
    /* Said once per display: useful when bringing a driver's planes up */
    if (compdata->planecount > 1)
        bug("[Compositor] display 0x%p: %lu scan-out planes\n", compdata->display, (unsigned long)compdata->planecount);
}

/*
 * The driver refused a commit. Planes are not used again on this display:
 * everything is composed into the primary, which is what one-plane drivers
 * do anyway. The overlays are switched off first (ignoring further errors).
 */
static void HIDDCompositorDisablePlanes(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode *n;
    struct SurfaceNode *sn;
    ULONG i, count = 0;
    struct HIDD_PlaneState *states = compdata->commitstates;

    bug("[Compositor] display 0x%p refused a plane commit: composing everything from now on\n", compdata->display);
    for (i = 0; i < compdata->planecount; i++)
    {
        if (compdata->planes[i].Type == vHidd_Plane_Overlay)
        {
            memset(&states[count], 0, sizeof(struct HIDD_PlaneState));
            states[count].Plane  = i;
            states[count].BitMap = NULL;
            count++;
        }
    }
    if (count)
        HIDD_Display_CommitPlanes(compdata->display, states, count, 0);
    compdata->planecount = 1;
    ForeachNode(&compdata->bitmapstack, n)
    {
        n->plane = -1;
        n->promoted = FALSE;
        ForeachNode(&n->surfaces, sn)
            sn->plane = -1;
    }
    HIDDCompositorToggleCompositing(compdata, TRUE);
}

/* Attributes of a surface's bitmap the plane logic needs, cached on the node */
static void HIDDCompositorSurfaceBitMapAttrs(struct HIDDCompositorData *compdata, struct SurfaceNode *sn)
{
    struct HIDD_CompositeRequest *req = SNREQ(sn);
    IPTR vf = 0, scanout = 0;

    if (req->BitMap)
    {
        OOP_GetAttr(req->BitMap, aHidd_BitMap_VideoFormat, &vf);
        OOP_GetAttr(req->BitMap, aHidd_BitMap_Scanout, &scanout);
    }
    sn->videofmt = vf;
    sn->scanout  = scanout ? TRUE : FALSE;
}

/* A surface the compositor can blit when it holds no plane: anything but a video bitmap */
#define SURFACE_BLITTABLE(sn) ((sn)->videofmt == vHidd_VideoFmt_None)

/* May this surface take a plane in the current pass? (plan 3.3 item 1) */
static BOOL HIDDCompositorSurfaceIsCandidate(struct HIDDCompositorData *compdata, struct SurfaceNode *sn)
{
    struct HIDD_CompositeRequest *req = SNREQ(sn);

    if (!sn->client || sn->denied || !sn->scanout || !req->BitMap)
        return FALSE;
    return (req->Flags & (CRF_PLANE_OK | CRF_PLANE_REQUIRED)) ? TRUE : FALSE;
}

/* Can this screen be scanned out by plane 'p' as it is (unscaled, at its offset)? */
static BOOL HIDDCompositorPlaneFits(struct HIDDCompositorData *compdata, ULONG p, struct StackBitMapNode *n)
{
    struct HIDD_PlaneInfo *pi = &compdata->planes[p];
    struct SurfaceNode *sn;
    OOP_Object *pf;
    IPTR stdpf = 0, w = 0, h = 0;

    if (pi->Type == vHidd_Plane_Cursor)
        return FALSE;
    if (n->haslayersurfaces || (n->sbmflags & COMPF_ALPHA))
        return FALSE;   /* composed layers and blending stay in software for now */
    /* A client surface the compositor may have to blit needs the screen in the composed bitmap */
    ForeachNode(&n->surfaces, sn)
    {
        if (sn->client && !sn->candidate && SURFACE_BLITTABLE(sn))
            return FALSE;
    }
    /*
     * graphics.library may still have the display's framebuffer installed in
     * the frontmost screen's BitMap from a passthrough LoadView(); that object
     * is the primary plane itself and cannot be an overlay source.
     */
    {
        OOP_Object *fb = NULL;
        OOP_GetAttr(compdata->display, aHidd_Display_FrameBuffer, (IPTR *)&fb);
        if (fb && (n->bm == fb))
            return FALSE;
    }
    if (!(pi->AlphaModes & (1 << vHidd_CompAlpha_None)))
        return FALSE;

    pf = (OOP_Object *)OOP_GET(n->bm, aHidd_BitMap_PixFmt);
    if (pf)
        OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
    if ((stdpf < 32) && !(pi->PixFmtMask & (1UL << stdpf)))
        return FALSE;

    OOP_GetAttr(n->bm, aHidd_BitMap_Width, &w);
    OOP_GetAttr(n->bm, aHidd_BitMap_Height, &h);
    if ((pi->MaxWidth && (w > pi->MaxWidth)) || (pi->MaxHeight && (h > pi->MaxHeight)))
        return FALSE;
    {
        /* Position constraints are about display pixels */
        struct Rectangle nr;

        HIDDCompositorNodeRect(n, &nr);
        if ((pi->PosAlignX > 1) && (nr.MinX % pi->PosAlignX))
            return FALSE;
        if ((pi->PosAlignY > 1) && (nr.MinY % pi->PosAlignY))
            return FALSE;
        if ((pi->Flags & PIF_FULL_WIDTH_ONLY) && (nr.MinX != 0))
            return FALSE;
    }
    if (pi->Flags & PIF_NEEDS_DISPLAYMEM)
    {
        /* Only bitmaps the driver allocated for display are in display memory */
        if (!OOP_GET(n->bm, aHidd_BitMap_Displayable))
            return FALSE;
    }
    if (n->scaled)
    {
        /* The plane has to scale the screen to its size on the display */
        ULONG sx = (ULONG)(((UQUAD)n->dstw << 16) / n->scrw);
        ULONG sy = (ULONG)(((UQUAD)n->dsth << 16) / n->scrh);

        if ((sx < pi->ScaleMin) || (sx > pi->ScaleMax) || (sy < pi->ScaleMin) || (sy > pi->ScaleMax))
            return FALSE;
    }
    return TRUE;
}

/*
 * Can plane 'p' show surface 'sn' of screen 'n' as it is visible now? On
 * success the plane geometry is left in sn->psrc/pdest/pflags.
 *
 * The visible region decides (plan 3.3 item 2): one rectangle is cropped to;
 * several need a destination colour key (the key is stamped into the primary
 * under the plane, so only a surface of a composed or passthrough screen can
 * use it); without a key an RGB surface is blitted by the compositor through
 * its region instead, and a video surface is shown over whatever covers it,
 * or hidden when the driver asks for that (PIF_HIDE_OBSCURED).
 */
static BOOL HIDDCompositorSurfacePlaneFits(struct HIDDCompositorData *compdata, ULONG p,
                                           struct StackBitMapNode *n, struct SurfaceNode *sn)
{
    struct HIDD_PlaneInfo *pi = &compdata->planes[p];
    struct HIDD_CompositeRequest *req = SNREQ(sn);
    struct RegionRectangle *rr;
    struct Rectangle full, vis;
    ULONG flags = 0, nrects = 0;
    LONG srcw, srch, dstw, dsth;

    if (pi->Type != vHidd_Plane_Overlay)
        return FALSE;
    if (!(pi->AlphaModes & (1UL << req->AlphaMode)))
        return FALSE;
    if (!sn->region || !(rr = sn->region->RegionRectangle))
        return FALSE;   /* nothing of it is visible: no plane */

    /* Format */
    if (sn->videofmt)
    {
        if ((sn->videofmt >= 32) || !(pi->VideoFmtMask & (1UL << sn->videofmt)))
            return FALSE;
    }
    else
    {
        OOP_Object *pf = (OOP_Object *)OOP_GET(req->BitMap, aHidd_BitMap_PixFmt);
        IPTR stdpf = 0;

        if (pf)
            OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
        if ((stdpf < 32) && !(pi->PixFmtMask & (1UL << stdpf)))
            return FALSE;
    }

    /* Source size */
    srcw = req->Src.MaxX - req->Src.MinX + 1;
    srch = req->Src.MaxY - req->Src.MinY + 1;
    if ((srcw <= 0) || (srch <= 0))
        return FALSE;
    if ((pi->MaxWidth && (srcw > pi->MaxWidth)) || (pi->MaxHeight && (srch > pi->MaxHeight)))
        return FALSE;

    /* The whole destination on the display, and what is visible of it */
    HIDDCompositorNodeMapRect(n, req->Dest.MinX, req->Dest.MinY, req->Dest.MaxX, req->Dest.MaxY, &full);
    dstw = full.MaxX - full.MinX + 1;
    dsth = full.MaxY - full.MinY + 1;
    if ((dstw <= 0) || (dsth <= 0))
        return FALSE;

    vis.MinX = rr->bounds.MinX + sn->region->bounds.MinX;
    vis.MinY = rr->bounds.MinY + sn->region->bounds.MinY;
    vis.MaxX = rr->bounds.MaxX + sn->region->bounds.MinX;
    vis.MaxY = rr->bounds.MaxY + sn->region->bounds.MinY;
    for (; rr; rr = rr->Next)
        nrects++;

    if (nrects > 1)
    {
        if ((pi->Flags & PIF_COLORKEY) && !n->promoted)
            flags |= PSF_COLORKEY;              /* the primary carries the key under the plane */
        else if (!sn->videofmt)
            return FALSE;                       /* RGB: blitted through its region instead */
        else if (pi->Flags & PIF_HIDE_OBSCURED)
            return FALSE;                       /* driver policy: off until it is one rectangle */
        /* Keyed, or a video shown on top: the whole rectangle, clipped to the display */
        if (!AndRectRect(&full, &compdata->displayrect, &vis))
            return FALSE;
    }

    /* Scaling: the plane scales Src onto the whole destination */
    if ((srcw != dstw) || (srch != dsth))
    {
        ULONG sx = (ULONG)(((UQUAD)dstw << 16) / srcw);
        ULONG sy = (ULONG)(((UQUAD)dsth << 16) / srch);

        if ((sx < pi->ScaleMin) || (sx > pi->ScaleMax) || (sy < pi->ScaleMin) || (sy > pi->ScaleMax))
            return FALSE;
    }

    /* Position constraints are about display pixels */
    if ((pi->PosAlignX > 1) && (vis.MinX % pi->PosAlignX))
        return FALSE;
    if ((pi->PosAlignY > 1) && (vis.MinY % pi->PosAlignY))
        return FALSE;
    if ((pi->Flags & PIF_FULL_WIDTH_ONLY) && (vis.MinX != 0))
        return FALSE;
    /* PIF_NEEDS_DISPLAYMEM: a scan-out bitmap is allocated by the driver for exactly this */

    /* Src cropped in proportion to the part of Dest that is shown (exact when unscaled) */
    sn->pdest = vis;
    sn->psrc.MinX = req->Src.MinX + (LONG)(((QUAD)(vis.MinX - full.MinX) * srcw) / dstw);
    sn->psrc.MinY = req->Src.MinY + (LONG)(((QUAD)(vis.MinY - full.MinY) * srch) / dsth);
    sn->psrc.MaxX = req->Src.MinX + (LONG)((((QUAD)(vis.MaxX - full.MinX + 1) * srcw) + dstw - 1) / dstw) - 1;
    sn->psrc.MaxY = req->Src.MinY + (LONG)((((QUAD)(vis.MaxY - full.MinY + 1) * srch) + dsth - 1) / dsth) - 1;
    if (sn->psrc.MaxX < sn->psrc.MinX)
        sn->psrc.MaxX = sn->psrc.MinX;
    if (sn->psrc.MaxY < sn->psrc.MinY)
        sn->psrc.MaxY = sn->psrc.MinY;
    if (sn->psrc.MaxX > req->Src.MaxX)
        sn->psrc.MaxX = req->Src.MaxX;
    if (sn->psrc.MaxY > req->Src.MaxY)
        sn->psrc.MaxY = req->Src.MaxY;
    sn->pflags = flags;
    return TRUE;
}

/*
 * One plane assignment pass over the stack (plan 3.3 items 4 and 5).
 *
 * (A) Which screens leave the composed bitmap for a plane of their own: the
 *     frontmost screens, for as long as each fits an overlay plane, never the
 *     backmost one (the primary is the base image of the display: a single
 *     screen is shown exactly as without planes, and a plane the hardware
 *     accepts but does not show can never leave the display blank). Z order
 *     must be kept: once a screen is composed every screen behind it is too,
 *     since a plane would show above the composed bitmap. Screens only get
 *     the planes no surface asks for (video before GL before screens), so an
 *     opening screen never takes a plane away from a video.
 * (B) A z pass back to front over the composed screens' surfaces, then each
 *     promoted screen and its surfaces: every candidate takes the lowest free
 *     plane whose z range fits above everything assigned below it. A surface
 *     that finds none is blitted (RGB) or off (video); a promoted screen that
 *     finds none, or whose blittable surface finds none, is composed instead
 *     and the pass starts over. Each restart demotes one screen, so it ends.
 *
 * The visible regions are recomputed in between: a promoted screen is not
 * part of the composed bitmap, and the surfaces' regions decide their planes.
 */
static void HIDDCompositorAssignPlanes(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode *n;
    struct SurfaceNode *sn;
    ULONG noverlay = 0, zfloor = 0, zcur, nreq, nok, used, usedcount, p;

    ForeachNode(&compdata->bitmapstack, n)
    {
        n->plane = -1;
        n->promoted = FALSE;
        ForeachNode(&n->surfaces, sn)
        {
            sn->plane  = -1;
            sn->pflags = 0;
            sn->candidate = HIDDCompositorSurfaceIsCandidate(compdata, sn);
        }
    }

    if (compdata->planecount < 2)
    {
        /* No planes: client surfaces are still placed (blitted) by their regions */
        if (compdata->nclientsurfaces)
            HIDDCompositorRecalculateVisibleRegions(compdata);
        return;
    }

    for (p = 0; p < compdata->planecount; p++)
    {
        if (compdata->planes[p].Type == vHidd_Plane_Overlay)
            noverlay++;
        else if ((compdata->planes[p].Type == vHidd_Plane_Primary) && (compdata->planes[p].ZMax > zfloor))
            zfloor = compdata->planes[p].ZMax;
    }

restart:
    /* (A) */
    nreq = nok = 0;
    ForeachNode(&compdata->bitmapstack, n)
    {
        n->promoted = FALSE;
        ForeachNode(&n->surfaces, sn)
        {
            if (sn->candidate)
            {
                if (SNREQ(sn)->Flags & CRF_PLANE_REQUIRED)
                    nreq++;
                else
                    nok++;
            }
        }
    }
    {
        ULONG budget = (noverlay > nreq + nok) ? noverlay - nreq - nok : 0;

        ForeachNode(&compdata->bitmapstack, n)
        {
            BOOL fits = FALSE;

            if (!n->n.mln_Succ->mln_Succ)
                break;      /* the backmost screen: primary */
            if (!budget || n->planedenied)
                break;
            for (p = 0; p < compdata->planecount; p++)
            {
                if ((compdata->planes[p].Type == vHidd_Plane_Overlay) && HIDDCompositorPlaneFits(compdata, p, n))
                {
                    fits = TRUE;
                    break;
                }
            }
            if (!fits)
                break;      /* composed, and so is every screen behind it */
            n->promoted = TRUE;
            budget--;
        }
    }

    /* Screens alone need no regions here (ToggleCompositing() computes them); surfaces are placed by theirs */
    if (compdata->nclientsurfaces)
    {
        HIDDCompositorRecalculateVisibleRegions(compdata);

        /* Only candidates with something visible compete for the planes below */
        nreq = nok = 0;
        ForeachNode(&compdata->bitmapstack, n)
        {
            ForeachNode(&n->surfaces, sn)
            {
                if (sn->candidate && sn->region && sn->region->RegionRectangle)
                {
                    if (SNREQ(sn)->Flags & CRF_PLANE_REQUIRED)
                        nreq++;
                    else
                        nok++;
                }
            }
        }
    }

    /* (B) */
    used = 0;
    usedcount = 0;
    zcur = zfloor;
    for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
         n->n.mln_Pred; n = (struct StackBitMapNode *)n->n.mln_Pred)
    {
        if (n->promoted)
        {
            LONG best = -1;
            ULONG bestz = 0;

            for (p = 0; p < compdata->planecount; p++)
            {
                struct HIDD_PlaneInfo *pi = &compdata->planes[p];
                ULONG z;

                if ((pi->Type != vHidd_Plane_Overlay) || (used & (1UL << p)))
                    continue;
                z = (zcur + 1 > pi->ZMin) ? zcur + 1 : pi->ZMin;
                if (z > pi->ZMax)
                    continue;
                if ((best >= 0) && (z >= bestz))
                    continue;
                if (!HIDDCompositorPlaneFits(compdata, p, n))
                    continue;
                best  = p;
                bestz = z;
            }
            if (best < 0)
            {
                n->planedenied = TRUE;
                goto restart;
            }
            n->plane  = best;
            n->zorder = bestz;
            used |= 1UL << best;
            usedcount++;
            zcur = bestz;
        }

        ForeachNode(&n->surfaces, sn)
        {
            LONG best = -1;
            ULONG bestz = 0, later;
            BOOL required;

            if (!sn->candidate || !sn->region || !sn->region->RegionRectangle)
                continue;   /* not a candidate, or nothing of it visible: no plane */
            required = (SNREQ(sn)->Flags & CRF_PLANE_REQUIRED) ? TRUE : FALSE;
            if (required)
            {
                nreq--;
                later = nreq;
            }
            else
            {
                nok--;
                later = nreq + nok;
            }
            /* Video before GL, and frontmost first within a class: leave planes for those still to come */
            if (noverlay - usedcount <= later)
                continue;

            for (p = 0; p < compdata->planecount; p++)
            {
                struct HIDD_PlaneInfo *pi = &compdata->planes[p];
                ULONG z;

                if ((pi->Type != vHidd_Plane_Overlay) || (used & (1UL << p)))
                    continue;
                z = (zcur + 1 > pi->ZMin) ? zcur + 1 : pi->ZMin;
                if (z > pi->ZMax)
                    continue;
                if ((best >= 0) && (z >= bestz))
                    continue;
                if (!HIDDCompositorSurfacePlaneFits(compdata, p, n, sn))
                    continue;
                best  = p;
                bestz = z;
            }
            if (best < 0)
            {
                struct HIDD_CompositeRequest *r = SNREQ(sn);

                /* Said on serial: a client surface that gets no plane is the first thing to look at when bringing a driver up */
                bug("[Compositor] surface 0x%p (video %lu, flags 0x%x) src %ldx%ld dest %ldx%ld region %s: no plane (%lu overlays, %lu used, z floor %lu)\n",
                    r, (unsigned long)sn->videofmt, r->Flags,
                    (long)(r->Src.MaxX - r->Src.MinX + 1), (long)(r->Src.MaxY - r->Src.MinY + 1),
                    (long)(r->Dest.MaxX - r->Dest.MinX + 1), (long)(r->Dest.MaxY - r->Dest.MinY + 1),
                    (sn->region->RegionRectangle->Next) ? "several rects" : "one rect",
                    (unsigned long)noverlay, (unsigned long)usedcount, (unsigned long)zcur);
                if (!required && n->promoted)
                {
                    /* It has to be blitted, which only the composed bitmap allows */
                    n->planedenied = TRUE;
                    goto restart;
                }
                continue;
            }
            HIDDCompositorSurfacePlaneFits(compdata, best, n, sn);   /* the geometry of the chosen plane */
            bug("[Compositor] surface 0x%p (video %lu) -> plane %ld z %lu dest (%d,%d)-(%d,%d) src (%d,%d)-(%d,%d)%s\n",
                SNREQ(sn), (unsigned long)sn->videofmt, (long)best, (unsigned long)bestz,
                sn->pdest.MinX, sn->pdest.MinY, sn->pdest.MaxX, sn->pdest.MaxY,
                sn->psrc.MinX, sn->psrc.MinY, sn->psrc.MaxX, sn->psrc.MaxY,
                (sn->pflags & PSF_COLORKEY) ? " keyed" : "");
            sn->plane  = best;
            sn->zorder = bestz;
            used |= 1UL << best;
            usedcount++;
            zcur = bestz;
            /* The plane shows it: nothing of the screen under it is needed */
            sn->opaque = TRUE;
            if (n->paintregion && sn->region)
                ClearRegionRegion(sn->region, n->paintregion);
        }
    }
}

/*
 * Fill in the plane states for the current assignment (screens and surfaces;
 * an unused overlay carries a NULL bitmap) and copy the overlay ones into
 * commitstates. Returns their number; *inuse is how many show something.
 */
static ULONG HIDDCompositorBuildPlaneStates(struct HIDDCompositorData *compdata, ULONG *inuse)
{
    struct StackBitMapNode *n;
    struct SurfaceNode *sn;
    ULONG i, count = 0, shown = 0;

    for (i = 0; i < compdata->planecount; i++)
    {
        struct HIDD_PlaneState *st = &compdata->planestates[i];

        if (compdata->planes[i].Type != vHidd_Plane_Overlay)
            continue;
        memset(st, 0, sizeof(*st));
        st->Plane = i;
    }

    ForeachNode(&compdata->bitmapstack, n)
    {
        struct HIDD_PlaneState *st;

        if (n->plane >= 0)
        {
            st = &compdata->planestates[n->plane];
            st->BitMap    = n->bm;
            st->Src.MinX  = 0;
            st->Src.MinY  = 0;
            st->Src.MaxX  = n->scrw - 1;
            st->Src.MaxY  = n->scrh - 1;
            HIDDCompositorNodeRect(n, &st->Dest);
            st->Opacity   = 0xFF;
            st->AlphaMode = vHidd_CompAlpha_None;
            st->ZOrder    = n->zorder;
            st->Flags     = 0;
            st->ColorKey  = 0;
            shown++;
        }
        ForeachNode(&n->surfaces, sn)
        {
            struct HIDD_CompositeRequest *req = SNREQ(sn);

            if (sn->plane < 0)
                continue;
            st = &compdata->planestates[sn->plane];
            st->BitMap    = req->BitMap;
            st->Src       = sn->psrc;
            st->Dest      = sn->pdest;
            st->Opacity   = req->Opacity;
            st->AlphaMode = req->AlphaMode;
            st->ZOrder    = sn->zorder;
            st->Flags     = sn->pflags;
            st->ColorKey  = (sn->pflags & PSF_COLORKEY) ? compdata->colorkey : 0;
            shown++;
        }
    }

    for (i = 0; i < compdata->planecount; i++)
    {
        if (compdata->planes[i].Type == vHidd_Plane_Overlay)
            compdata->commitstates[count++] = compdata->planestates[i];
    }
    *inuse = shown;
    return count;
}

/*
 * Plane planning with the driver's test commit (plan 3.3 item 7). The static
 * checks of the Fits() functions cannot know the hardware's dynamic budget,
 * so the assignment is offered with fHidd_Commit_Test and, when refused,
 * reduced by one plane user and offered again until it passes (an empty set
 * passes without asking). The victim is, in this order, the first that
 * exists: the backmost screen on a plane (composed; a composed screen sits at
 * the primary's z and must not have an overlay screen below it), the backmost
 * CRF_PLANE_OK surface on a plane (blitted), the backmost CRF_PLANE_REQUIRED
 * surface on a plane (off, PlaneLost). Each victim is marked denied for this
 * pass and AssignPlanes() is re-run so z ranges stay consistent. This runs
 * before ToggleCompositing(), which depends on which screens are on planes;
 * DisablePlanes() is left for a real commit failing after its test passed.
 */
static void HIDDCompositorPlanPlanes(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode *n;
    struct SurfaceNode *sn;

    ForeachNode(&compdata->bitmapstack, n)
    {
        n->planedenied = FALSE;
        ForeachNode(&n->surfaces, sn)
            sn->denied = FALSE;
    }

    for (;;)
    {
        struct StackBitMapNode *victim = NULL;
        struct SurfaceNode *svictim = NULL;
        ULONG count, inuse, pass;

        HIDDCompositorAssignPlanes(compdata);
        if (compdata->planecount < 2)
            return;
        count = HIDDCompositorBuildPlaneStates(compdata, &inuse);
        if (!inuse)
            return;
        if (HIDD_Display_CommitPlanes(compdata->display, compdata->commitstates, count, fHidd_Commit_Test))
            return;

        for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
             n->n.mln_Pred; n = (struct StackBitMapNode *)n->n.mln_Pred)
        {
            if (n->plane >= 0)
            {
                victim = n;
                break;
            }
        }
        if (victim)
        {
            D(bug("[Compositor] %s: test commit refused, screen 0x%p composed\n", __func__, victim->bm));
            victim->planedenied = TRUE;
            continue;
        }
        for (pass = 0; (pass < 2) && !svictim; pass++)
        {
            for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
                 n->n.mln_Pred && !svictim; n = (struct StackBitMapNode *)n->n.mln_Pred)
            {
                ForeachNode(&n->surfaces, sn)
                {
                    BOOL required = (SNREQ(sn)->Flags & CRF_PLANE_REQUIRED) ? TRUE : FALSE;

                    if ((sn->plane >= 0) && (required == (pass == 1)))
                    {
                        svictim = sn;
                        break;
                    }
                }
            }
        }
        if (!svictim)
            return;     /* nothing holds a plane, yet refused: nothing more to take away */
        D(bug("[Compositor] %s: test commit refused, surface 0x%p %s\n", __func__, SNREQ(svictim),
              (SNREQ(svictim)->Flags & CRF_PLANE_REQUIRED) ? "off" : "blitted"));
        svictim->denied = TRUE;
    }
}

/*
 * Is the display driven by something other than the frontmost bitmap alone?
 * graphics.library installs the display's framebuffer into the frontmost
 * screen's BitMap when this says FALSE (passthrough mirroring); that is only
 * right when the framebuffer scans that one bitmap out. A composed bitmap or
 * a screen on a plane both mean the screens are shown from their own memory.
 */
static BOOL HIDDCompositorIsActive(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode *n;

    if (compdata->displaybitmap)
        return TRUE;
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->plane >= 0)
            return TRUE;
    }
    return FALSE;
}

/*
 * Program the overlay planes from the stack (the primary is driven by
 * Show()). The clients' PlaneGained/PlaneLost are queued after the real
 * commit only, never from the test loop.
 */
static void HIDDCompositorCommitPlanes(struct HIDDCompositorData *compdata, ULONG flags, struct HookFires *fires)
{
    if (compdata->planecount >= 2)
    {
        ULONG inuse, count = HIDDCompositorBuildPlaneStates(compdata, &inuse);

        if (count && !HIDD_Display_CommitPlanes(compdata->display, compdata->commitstates, count, flags))
            HIDDCompositorDisablePlanes(compdata);
    }
    HIDDCompositorQueuePlaneHooks(compdata, fires);
}

/****************************************************************************************/
/* Composited layer surfaces                                                            */
/****************************************************************************************/

static void HIDDCompositorFreeSurfaceNode(struct HIDDCompositorData *compdata, struct SurfaceNode *sn)
{
    if (sn->client)
        sn->client->sn = NULL;      /* the registry entry stays until DetachSurface */
    if (sn->region)
        DisposeRegion(sn->region);
    FreeMem(sn, sizeof(struct SurfaceNode));
}

/* Free every surface node of a screen (layer and client ones) */
static void HIDDCompositorFreeSurfaces(struct HIDDCompositorData *compdata, struct StackBitMapNode *n)
{
    struct SurfaceNode *sn, *next;

    ForeachNodeSafe(&n->surfaces, sn, next)
        HIDDCompositorFreeSurfaceNode(compdata, sn);
    NEWLIST(&n->surfaces);
    n->hassurfaces = FALSE;
    n->haslayersurfaces = FALSE;
}

/*
 * Take a private copy of a screen's layer surface list (owned by
 * layers.library, valid only during the call that hands it over). The
 * screen's client surfaces are kept, behind the layer surfaces.
 */
static void HIDDCompositorCopySurfaces(struct HIDDCompositorData *compdata, struct StackBitMapNode *n,
                                       struct HIDD_CompositeRequest *req)
{
    struct SurfaceNode *sn, *next;
    struct MinList clients;

    NEWLIST(&clients);
    ForeachNodeSafe(&n->surfaces, sn, next)
    {
        if (sn->client)
        {
            Remove((struct Node *)sn);
            AddTail((struct List *)&clients, (struct Node *)sn);
        }
    }
    HIDDCompositorFreeSurfaces(compdata, n);

    for (; req; req = req->Next)
    {
        sn = AllocMem(sizeof(struct SurfaceNode), MEMF_ANY | MEMF_CLEAR);
        if (!sn)
            break;
        sn->req = *req;
        sn->req.Next = NULL;
        sn->plane = -1;
        HIDDCompositorSurfaceBitMapAttrs(compdata, sn);
        sn->opaque = (req->Flags & CRF_OPAQUE) && (req->Opacity == 0xFF) && !req->Shape;
        AddTail((struct List *)&n->surfaces, (struct Node *)sn);
        n->haslayersurfaces = TRUE;
    }

    while ((sn = (struct SurfaceNode *)RemHead((struct List *)&clients)) != NULL)
        AddTail((struct List *)&n->surfaces, (struct Node *)sn);
    n->hassurfaces = IsListEmpty(&n->surfaces) ? FALSE : TRUE;
}

/* Union of a screen's layer surface regions, display relative */
static void HIDDCompositorOrSurfaceRegions(struct HIDDCompositorData *compdata, struct StackBitMapNode *n, struct Region *into)
{
    struct SurfaceNode *sn;

    ForeachNode(&n->surfaces, sn)
    {
        if (sn->region && !sn->client)
            OrRegionRegion(sn->region, into);
    }
}

/****************************************************************************************/
/* Client surfaces (video overlays, 3D colour buffers): see VIDEO-OVERLAY-3D-SCANOUT-PLAN */
/****************************************************************************************/

static struct ClientSurface *HIDDCompositorFindClientSurface(struct HIDDCompositorData *compdata, struct HIDD_CompositeRequest *req)
{
    struct ClientSurface *cs;

    ForeachNode(&compdata->clientsurfaces, cs)
    {
        if (cs->req == req)
            return cs;
    }
    return NULL;
}

/* Put a client surface on its screen's list (behind the layer surfaces, attach order) */
static struct SurfaceNode *HIDDCompositorLinkClientSurface(struct HIDDCompositorData *compdata, struct StackBitMapNode *n,
                                                           struct ClientSurface *cs)
{
    struct SurfaceNode *sn = AllocMem(sizeof(struct SurfaceNode), MEMF_ANY | MEMF_CLEAR);

    if (!sn)
        return NULL;
    sn->creq   = cs->req;
    sn->client = cs;
    sn->plane  = -1;
    HIDDCompositorSurfaceBitMapAttrs(compdata, sn);
    AddTail((struct List *)&n->surfaces, (struct Node *)sn);
    n->hassurfaces = TRUE;
    cs->sn = sn;
    return sn;
}

static void HIDDCompositorUnlinkClientSurface(struct HIDDCompositorData *compdata, struct StackBitMapNode *n, struct SurfaceNode *sn)
{
    Remove((struct Node *)sn);
    HIDDCompositorFreeSurfaceNode(compdata, sn);
    n->hassurfaces = IsListEmpty(&n->surfaces) ? FALSE : TRUE;
}

/*
 * After a stack change: the screens' nodes are new, so every client surface
 * whose screen is on the stack gets its node again (a screen that left the
 * display keeps its client surfaces until it is shown again or they detach).
 */
static void HIDDCompositorRelinkClientSurfaces(struct HIDDCompositorData *compdata)
{
    struct ClientSurface *cs;

    ForeachNode(&compdata->clientsurfaces, cs)
    {
        struct StackBitMapNode *n = HIDDCompositorFindBitMapStackNode(compdata, cs->screenbm);

        cs->sn = NULL;
        if (n)
            HIDDCompositorLinkClientSurface(compdata, n, cs);
    }
}

/* Lock held: Damage for every client surface of 'n' whose Dest meets the rectangle (screen bitmap coordinates) */
static void HIDDCompositorQueueDamage(struct HIDDCompositorData *compdata, struct StackBitMapNode *n,
                                      WORD x, WORD y, WORD width, WORD height, struct HookFires *fires)
{
    struct SurfaceNode *sn;
    struct Rectangle dmg, r;

    dmg.MinX = x;
    dmg.MinY = y;
    dmg.MaxX = x + width - 1;
    dmg.MaxY = y + height - 1;
    ForeachNode(&n->surfaces, sn)
    {
        if (sn->client && AndRectRect(&dmg, &sn->creq->Dest, &r))
            HIDDCompositorQueueHook(compdata, fires, sn->client, vHidd_SurfaceHook_Damage, &r, 0);
    }
}

/* Lock held, after a real commit: PlaneGained/PlaneLost for every client surface whose plane state changed */
static void HIDDCompositorQueuePlaneHooks(struct HIDDCompositorData *compdata, struct HookFires *fires)
{
    struct ClientSurface *cs;

    ForeachNode(&compdata->clientsurfaces, cs)
    {
        LONG cur = (cs->sn && (cs->sn->plane >= 0)) ? cs->sn->plane : -1;

        if (cur == cs->hookplane)
            continue;
        cs->hookplane = cur;
        if (cur >= 0)
            HIDDCompositorQueueHook(compdata, fires, cs, vHidd_SurfaceHook_PlaneGained, NULL, cur);
        else
            HIDDCompositorQueueHook(compdata, fires, cs, vHidd_SurfaceHook_PlaneLost, NULL, 0);
    }
}

/* Paint the display's colour key over 'rect' of 'target': a keyed plane shows its surface there */
static void HIDDCompositorFillKey(struct HIDDCompositorData *compdata, OOP_Object *target, struct Rectangle *rect)
{
    HIDDT_Color col;
    HIDDT_Pixel pix;
    IPTR oldfg = 0;

    col.red   = ((compdata->colorkey >> 16) & 0xFF) * 0x0101;
    col.green = ((compdata->colorkey >> 8) & 0xFF) * 0x0101;
    col.blue  = (compdata->colorkey & 0xFF) * 0x0101;
    col.alpha = 0xFFFF;
    pix = HIDD_BM_MapColor(target, &col);

    OOP_GetAttr(compdata->gc, aHidd_GC_Foreground, &oldfg);
    OOP_SetAttrsTags(compdata->gc, aHidd_GC_Foreground, pix, TAG_DONE);
    HIDD_BM_FillRect(target, compdata->gc, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);
    OOP_SetAttrsTags(compdata->gc, aHidd_GC_Foreground, oldfg, TAG_DONE);
}

/*
 * Colour key stamping in passthrough (plan 3.3 item 3): the screen bitmap
 * itself is the primary, so the key for a keyed surface plane is painted into
 * it over the surface's visible region. The compositor never restores what
 * it painted over; the client's backfill keeps the area keyed afterwards and
 * the application repaints it when the surface goes. Composed screens get
 * their key from the surface redraw (HIDDCompositorRedrawSurfaceRect).
 */
static void HIDDCompositorStampKeys(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode *n;
    struct SurfaceNode *sn;

    if (compdata->displaybitmap || !compdata->passthroughbm)
        return;
    n = HIDDCompositorFindBitMapStackNode(compdata, compdata->passthroughbm);
    if (!n || !n->hassurfaces)
        return;

    ForeachNode(&n->surfaces, sn)
    {
        struct RegionRectangle *rr;

        if (!sn->client || (sn->plane < 0) || !(sn->pflags & PSF_COLORKEY) || !sn->region)
            continue;
        for (rr = sn->region->RegionRectangle; rr; rr = rr->Next)
        {
            struct Rectangle r;

            /* Display -> screen bitmap coordinates (a passthrough screen is unscaled) */
            r.MinX = rr->bounds.MinX + sn->region->bounds.MinX - n->leftedge;
            r.MinY = rr->bounds.MinY + sn->region->bounds.MinY - n->topedge;
            r.MaxX = rr->bounds.MaxX + sn->region->bounds.MinX - n->leftedge;
            r.MaxY = rr->bounds.MaxY + sn->region->bounds.MinY - n->topedge;
            HIDDCompositorFillKey(compdata, n->bm, &r);
            HIDD_BM_UpdateRect(n->bm, r.MinX, r.MinY, r.MaxX - r.MinX + 1, r.MaxY - r.MinY + 1);
        }
    }
}

/****************************************************************************************/
/* Surface regions and drawing                                                          */
/****************************************************************************************/

/*
 * Compute the display relative region of every surface of a screen (its Dest
 * rectangle or Shape, moved by the screen's offset and clipped to what is
 * visible of the screen), and the screen's paint region (what remains of the
 * screen once opaque surfaces are taken out).
 */
static void HIDDCompositorRecalculateSurfaces(struct HIDDCompositorData *compdata, struct StackBitMapNode *n)
{
    struct SurfaceNode *sn;

    if (n->paintregion)
        ClearRegion(n->paintregion);
    else
        n->paintregion = NewRegion();
    if (n->paintregion && n->screenregion)
        OrRegionRegion(n->screenregion, n->paintregion);

    ForeachNode(&n->surfaces, sn)
    {
        struct HIDD_CompositeRequest *req = SNREQ(sn);
        struct Rectangle r;

        if (sn->region)
            ClearRegion(sn->region);
        else
            sn->region = NewRegion();
        if (!sn->region)
            continue;

        if (req->Shape)
        {
            struct RegionRectangle *rr;

            for (rr = req->Shape->RegionRectangle; rr; rr = rr->Next)
            {
                HIDDCompositorNodeMapRect(n,
                    rr->bounds.MinX + req->Shape->bounds.MinX, rr->bounds.MinY + req->Shape->bounds.MinY,
                    rr->bounds.MaxX + req->Shape->bounds.MinX, rr->bounds.MaxY + req->Shape->bounds.MinY, &r);
                OrRectRegion(sn->region, &r);
            }
        }
        else
        {
            HIDDCompositorNodeMapRect(n, req->Dest.MinX, req->Dest.MinY, req->Dest.MaxX, req->Dest.MaxY, &r);
            OrRectRegion(sn->region, &r);
        }

        if (n->promoted)
        {
            /*
             * The screen is on a plane: the surface shows where the screen is
             * on the display (what is in front of the screen is on planes
             * above it), still display coordinates.
             */
            struct Rectangle clip;

            HIDDCompositorNodeRect(n, &clip);
            if (AndRectRect(&clip, &compdata->displayrect, &clip))
                AndRectRegion(sn->region, &clip);
            else
                ClearRegion(sn->region);
        }
        else if (n->screenregion)
        {
            /* Only what the screen shows of it */
            AndRegionRegion(n->screenregion, sn->region);
        }

        if (sn->client)
        {
            /*
             * On a plane nothing under it is needed. Blitted, the request
             * says whether it covers the screen. A video surface without a
             * plane shows nothing: the screen's own pixels stay.
             */
            sn->opaque = (sn->plane >= 0) ||
                         ((req->Flags & CRF_OPAQUE) && (req->Opacity == 0xFF) && !req->Shape && SURFACE_BLITTABLE(sn));
        }

        /* An opaque surface hides the screen pixels under it: no need to blit those */
        if (sn->opaque && n->paintregion)
            ClearRegionRegion(sn->region, n->paintregion);
    }
}

/* Blit the part of a surface inside 'rect' (display relative) onto the render target */
static void HIDDCompositorRedrawSurfaceRect(struct HIDDCompositorData *compdata, OOP_Object *renderTarget,
                                            struct StackBitMapNode *n, struct SurfaceNode *sn, struct Rectangle *rect)
{
    struct HIDD_CompositeRequest *req = SNREQ(sn);
    /* Display -> surface bitmap coordinates */
    WORD sx = req->Src.MinX + (rect->MinX - (req->Dest.MinX + n->leftedge));
    WORD sy = req->Src.MinY + (rect->MinY - (req->Dest.MinY + n->topedge));
    ULONG w = rect->MaxX - rect->MinX + 1;
    ULONG h = rect->MaxY - rect->MinY + 1;
    BOOL copy = sn->opaque || (req->AlphaMode == vHidd_CompAlpha_None && req->Opacity == 0xFF);

    if (sn->plane >= 0)
    {
        /* A plane shows it: with a colour key the primary carries the key under it, otherwise nothing is drawn */
        if (sn->pflags & PSF_COLORKEY)
            HIDDCompositorFillKey(compdata, renderTarget, rect);
        return;
    }
    if (!SURFACE_BLITTABLE(sn))
        return;     /* a video bitmap is never blitted: gfx.hidd has no conversion for it */

    if (n->scaled)
    {
        /* The surface scales with its screen */
        struct Rectangle d;

        HIDDCompositorNodeMapRect(n, req->Dest.MinX, req->Dest.MinY, req->Dest.MaxX, req->Dest.MaxY, &d);
        HIDDCompositorScaledBlit(compdata, renderTarget, req->BitMap,
                                 req->Src.MinX, req->Src.MinY,
                                 req->Dest.MaxX - req->Dest.MinX + 1, req->Dest.MaxY - req->Dest.MinY + 1,
                                 d.MinX, d.MinY, d.MaxX - d.MinX + 1, d.MaxY - d.MinY + 1,
                                 rect, req->Opacity, !copy);
        return;
    }

    if (copy)
        HIDD_Gfx_CopyBox(compdata->gfx, req->BitMap, sx, sy, renderTarget, rect->MinX, rect->MinY, w, h, compdata->gc);
    else
        HIDD_Gfx_CopyBoxAlpha(compdata->gfx, req->BitMap, sx, sy, renderTarget, rect->MinX, rect->MinY, w, h,
                              req->Opacity, compdata->gc);
}

/* Draw every surface of a screen (back to front) that intersects 'drawrect' (NULL = all) */
static void HIDDCompositorRedrawSurfaces(struct HIDDCompositorData *compdata, OOP_Object *renderTarget,
                                         struct StackBitMapNode *n, struct Rectangle *drawrect)
{
    struct SurfaceNode *sn;

    ForeachNode(&n->surfaces, sn)
    {
        struct RegionRectangle *rr;

        if (!sn->region)
            continue;

        for (rr = sn->region->RegionRectangle; rr; rr = rr->Next)
        {
            struct Rectangle r;

            r.MinX = rr->bounds.MinX + sn->region->bounds.MinX;
            r.MinY = rr->bounds.MinY + sn->region->bounds.MinY;
            r.MaxX = rr->bounds.MaxX + sn->region->bounds.MinX;
            r.MaxY = rr->bounds.MaxY + sn->region->bounds.MinY;

            if (!drawrect || AndRectRect(drawrect, &r, &r))
            {
                HIDDCompositorRedrawSurfaceRect(compdata, renderTarget, n, sn, &r);
                if (renderTarget == compdata->displaybitmap)
                    HIDD_BM_UpdateRect(compdata->displaybitmap, r.MinX, r.MinY,
                                       r.MaxX - r.MinX + 1, r.MaxY - r.MinY + 1);
            }
        }
    }
}

/* Find the surface (layer or client) showing bitmap 'bm', and its screen */
static struct SurfaceNode *HIDDCompositorFindSurface(struct HIDDCompositorData *compdata, OOP_Object *bm,
                                                     struct StackBitMapNode **screen)
{
    struct StackBitMapNode *n;
    struct SurfaceNode *sn;

    ForeachNode(&compdata->bitmapstack, n)
    {
        if (!n->hassurfaces)
            continue;
        ForeachNode(&n->surfaces, sn)
        {
            if (SNREQ(sn)->BitMap == bm)
            {
                *screen = n;
                return sn;
            }
        }
    }
    return NULL;
}

/****************************************************************************************/
/* Rendering                                                                            */
/****************************************************************************************/

static inline void HIDDCompositorRedrawBitmap(struct HIDDCompositorData *compdata, OOP_Object *renderTarget, struct StackBitMapNode *n, struct Rectangle *rect)
{
    /* The given rectangle is already in screen coordinate system here */
    ULONG blitwidth  = rect->MaxX - rect->MinX + 1;
    ULONG blitheight = rect->MaxY - rect->MinY + 1;

    DREDRAWBM(bug("[Compositor] %s: Redraw HiddBitMap 0x%p, Rect[%d, %d - %d, %d]\n", __func__, n->bm,
                      rect->MinX, rect->MinY, rect->MaxX, rect->MaxY));

    if (renderTarget && n->scaled)
    {
        struct Rectangle nr;

        HIDDCompositorNodeRect(n, &nr);
        HIDDCompositorScaledBlit(compdata, renderTarget, n->bm, 0, 0, n->scrw, n->scrh,
                                 nr.MinX, nr.MinY, n->dstw, n->dsth, rect, 0xFF,
                                 (n->sbmflags & COMPF_ALPHA) ? TRUE : FALSE);
    }
    else if (renderTarget)
    {
        if (!(n->sbmflags & COMPF_ALPHA))
        {
            HIDD_Gfx_CopyBox(compdata->gfx, n->bm,
                            /* Transform to source bitmap coord system */
                            rect->MinX - n->leftedge, rect->MinY - n->topedge,
                            renderTarget,
                            rect->MinX, rect->MinY, blitwidth, blitheight,
                            compdata->gc);
        }
        else
        {
            /*
             * Blend the source over the render target. CopyBoxAlpha() reads
             * the source in its own pixel format through the driver, so it
             * neither needs direct access to the source memory nor assumes a
             * byte order for it.
             */
            HIDD_Gfx_CopyBoxAlpha(compdata->gfx, n->bm,
                            rect->MinX - n->leftedge, rect->MinY - n->topedge,
                            renderTarget,
                            rect->MinX, rect->MinY, blitwidth, blitheight,
                            0xFF, compdata->gc);
        }
    }
}

static inline void HIDDCompositorFillRect(struct HIDDCompositorData *compdata, OOP_Object *renderTarget, ULONG MinX, ULONG MinY, ULONG MaxX, ULONG MaxY)
{
    DREDRAWSCR(bug("[Compositor] %s: Filling [%d, %d - %d, %d]\n", __func__,
                   MinX, MinY, MaxX, MaxY));

    HIDD_BM_FillRect(renderTarget, compdata->gc,
                     MinX, MinY, MaxX, MaxY);
}

static VOID HIDDCompositorRedrawAlphaRegions(struct HIDDCompositorData *compdata, struct Rectangle *drawrect)
{
    OOP_Object          *renderTarget = compdata->displaybitmap;
    struct Rectangle    alpharect;
    struct StackBitMapNode *n;
    struct BitMap       *backbm;

    if (compdata->intermedbitmap)
        renderTarget = compdata->intermedbitmap;

    DREDRAWSCR(if (drawrect){ bug("[Compositor] %s: Rect [%d, %d - %d, %d]\n", __func__, _RECT((*drawrect))); })

    OOP_GetAttr(renderTarget, aHidd_BitMap_BMStruct, (IPTR *)&backbm);

    // Alpha Regions are drawn in reverse order incase they overlap..
    for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
        n->n.mln_Pred; n = (struct StackBitMapNode *)n->n.mln_Pred)
    {
        if ((n->sbmflags & STACKNODEF_VISIBLE) &&
            (n->sbmflags & COMPF_ALPHA) &&
            (n->screenregion))
        {
            struct RegionRectangle *srrect;

            if ((srrect = n->screenregion->RegionRectangle) != NULL)
            {
                while (srrect)
                {
                    alpharect.MinX = srrect->bounds.MinX + n->screenregion->bounds.MinX;
                    alpharect.MinY = srrect->bounds.MinY + n->screenregion->bounds.MinY;
                    alpharect.MaxX = srrect->bounds.MaxX + n->screenregion->bounds.MinX;
                    alpharect.MaxY = srrect->bounds.MaxY + n->screenregion->bounds.MinY;

                    if (!(drawrect) || AndRectRect(drawrect, &alpharect, &alpharect))
                    {
                        if ((n->prealphacomphook) && (backbm != NULL))
                        {
                            struct HIDD_BackFillHookMsg preprocessmsg;
                            preprocessmsg.bounds = &alpharect;
                            preprocessmsg.offsetx = 0;
                            preprocessmsg.offsety = 0;
                            comp_CallHook(n->prealphacomphook, backbm, &preprocessmsg);
                        }

                        HIDDCompositorRedrawBitmap(compdata, renderTarget, n, &alpharect);
                        if (renderTarget == compdata->displaybitmap)
                            HIDD_BM_UpdateRect(compdata->displaybitmap,
                               alpharect.MinX, alpharect.MinY,
                               alpharect.MaxX - alpharect.MinX + 1,
                               alpharect.MaxY - alpharect.MinY + 1);
                    }
                    srrect = srrect->Next;
                }
            }
            if (n->hassurfaces)
                HIDDCompositorRedrawSurfaces(compdata, renderTarget, n, drawrect);
        }
    }
}

static VOID HIDDCompositorRedrawVisibleRegions(struct HIDDCompositorData *compdata, struct Rectangle *drawrect)
{
    OOP_Object          *renderTarget = compdata->displaybitmap;
    struct Region       *dispvisregion = NULL;
    struct Rectangle    tmprect;
    struct BitMap       *clearbm;
    struct StackBitMapNode *n;

    DREDRAWSCR(bug("[Compositor] %s: Redrawing Display\n", __func__));

    if (!renderTarget)
        return;

    if (!(drawrect))
    {
        /* Recalculate visible regions */
        HIDDCompositorRecalculateVisibleRegions(compdata);
    }

    if ((compdata->flags & COMPSTATEF_HASALPHA) && (compdata->intermedbitmap))
        renderTarget = compdata->intermedbitmap;

    if ((dispvisregion = NewRegion()) != NULL)
    {
        struct RegionRectangle *dispclrrect;

        if (drawrect)
            OrRectRegion(dispvisregion, drawrect);
        else
            OrRectRegion(dispvisregion, &compdata->displayrect);

        ForeachNode(&compdata->bitmapstack, n)
        {
            if ((n->sbmflags & STACKNODEF_VISIBLE) &&
                (!(n->sbmflags & COMPF_ALPHA)) &&
                (n->screenregion))
            {
                struct RegionRectangle *srrect;
                /* Opaque surfaces cover part of the screen: those pixels are not blitted */
                struct Region *paint = (n->hassurfaces && n->paintregion) ? n->paintregion : n->screenregion;

                // Render the visable regions ..
                if ((srrect = paint->RegionRectangle) != NULL)
                {
                    while (srrect)
                    {
                        tmprect.MinX = srrect->bounds.MinX + paint->bounds.MinX;
                        tmprect.MinY = srrect->bounds.MinY + paint->bounds.MinY;
                        tmprect.MaxX = srrect->bounds.MaxX + paint->bounds.MinX;
                        tmprect.MaxY = srrect->bounds.MaxY + paint->bounds.MinY;

                        if (!(drawrect) || AndRectRect(drawrect, &tmprect, &tmprect))
                        {
                            HIDDCompositorRedrawBitmap(compdata, renderTarget, n, &tmprect);
                            if (renderTarget == compdata->displaybitmap)
                                HIDD_BM_UpdateRect(compdata->displaybitmap,
                                   tmprect.MinX, tmprect.MinY,
                                   tmprect.MaxX - tmprect.MinX + 1,
                                   tmprect.MaxY - tmprect.MinY + 1);
                        }
                        srrect = srrect->Next;
                    }
                }
                if (n->hassurfaces)
                    HIDDCompositorRedrawSurfaces(compdata, renderTarget, n, drawrect);
                ClearRegionRegion(n->screenregion, dispvisregion);
            }
        }
        OOP_GetAttr(renderTarget, aHidd_BitMap_BMStruct, (IPTR *)&clearbm);

        dispclrrect = dispvisregion->RegionRectangle;
        while (dispclrrect)
        {
            struct HIDD_BackFillHookMsg clearmsg;

            tmprect.MinX = dispclrrect->bounds.MinX + dispvisregion->bounds.MinX;
            tmprect.MinY = dispclrrect->bounds.MinY + dispvisregion->bounds.MinY;
            tmprect.MaxX = dispclrrect->bounds.MaxX + dispvisregion->bounds.MinX;
            tmprect.MaxY = dispclrrect->bounds.MaxY + dispvisregion->bounds.MinY;

            if (!(drawrect) || AndRectRect(drawrect, &tmprect, &tmprect))
            {
                if (clearbm && (compdata->backfillhook != &compdata->defaultbackfill))
                {
                    clearmsg.bounds = &tmprect;
                    clearmsg.offsetx = 0;
                    clearmsg.offsety = 0;
                    comp_CallHook(compdata->backfillhook, clearbm, &clearmsg);
                }
                else
                    HIDDCompositorFillRect(compdata, renderTarget, tmprect.MinX, tmprect.MinY, tmprect.MaxX, tmprect.MaxY);

                if (renderTarget == compdata->displaybitmap)
                    HIDD_BM_UpdateRect(compdata->displaybitmap,
                       tmprect.MinX, tmprect.MinY,
                       tmprect.MaxX - tmprect.MinX + 1,
                       tmprect.MaxY - tmprect.MinY + 1);
            }
            dispclrrect = dispclrrect->Next;
        }
        if (compdata->flags & COMPSTATEF_HASALPHA)
        {
            HIDDCompositorRedrawAlphaRegions(compdata, drawrect);
        }
        DisposeRegion(dispvisregion);
    }
    if (renderTarget != compdata->displaybitmap)
    {
        struct Rectangle *r = drawrect ? drawrect : &compdata->displayrect;

        HIDD_Gfx_CopyBox(compdata->gfx, renderTarget,
                r->MinX, r->MinY,
                compdata->displaybitmap,
                r->MinX, r->MinY,
                r->MaxX - r->MinX + 1,
                r->MaxY - r->MinY + 1,
                compdata->gc);
        HIDD_BM_UpdateRect(compdata->displaybitmap,
                r->MinX, r->MinY,
                r->MaxX - r->MinX + 1,
                r->MaxY - r->MinY + 1);
    }
}

/*
 * Allocate a bitmap of the composition mode through graphics.library, so it
 * carries a struct BitMap for the backfill / pre-alpha hooks. The display's
 * public ID base makes a complete DisplayID out of the HIDD mode.
 */
static OOP_Object *HIDDCompositorAllocBitMap(struct HIDDCompositorData *compdata, ULONG flags)
{
    struct TagItem bmtags[2];
    struct BitMap *bm;
    IPTR displayid = 0;

    OOP_GetAttr(compdata->display, aHidd_Display_DisplayID, &displayid);

    bmtags[0].ti_Tag = BMATags_DisplayID; bmtags[0].ti_Data = displayid | compdata->displaymode;
    bmtags[1].ti_Tag = TAG_DONE;          bmtags[1].ti_Data = 0;

    bm = AllocBitMap(compdata->displayrect.MaxX - compdata->displayrect.MinX + 1,
                     compdata->displayrect.MaxY - compdata->displayrect.MinY + 1,
                     compdata->displaydepth, flags | BMF_CHECKVALUE, (struct BitMap *)bmtags);
    if (!bm)
        return NULL;

    return HIDD_BM_OBJ(bm);
}

static void HIDDCompositorFreeBitMap(struct HIDDCompositorData *compdata, OOP_Object *bm)
{
    struct BitMap *freebm = NULL;

    OOP_GetAttr(bm, aHidd_BitMap_BMStruct, (IPTR *)&freebm);
    if (freebm)
        FreeBitMap(freebm);
    else
        OOP_DisposeObject(bm);
}

/*

There are several cases that needs to be handled in this code. They are documented
below. Please read it before making changes.
etb     = existing topbitmap
ntb     = new top bitmap
sb      = screen bitmap
cb      = composited bitmap
fs      = covers full screen
nfs     = not covers full screen
mA      = mode "A"
mB      = mode "B"
disp()  = dispose
new()   = new

The resulting mode is always that of screen bitmap as set in "effect" column.
The composited bitmap always matches the resulting screen mode or is NULL.

| exiting screen situation          | change         | effect                               |
| USE CASE: SWITCHING BETWEEN FULL SCREEN SCREENS - SAME MODE                               |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->fs, mA    | sb==ntb, cb==NULL                    |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->fs, mA    | sb==ntb, cb!=NULL                    |
| USE CASE: SWITCHING BETWEEN FULL SCREEN AND NOT FULL SCREEN SCREENS - SAME MODE           |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->nfs, mA   | new(cb), sb==cb, cb!=NULL            |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->nfs, mA   | sb==cb, cb!=NULL                     |
| USE CASE: SWITCHING BETWEEN NOT FULL SCREEN AND FULL SCREEN SCREENS - SAME MODE           |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->fs, mA    | sb==ntb, cb!=NULL                    |
| USE CASE: SWITCHING BETWEEN NOT FULL SCREEN AND NOT FULL SCREEN SCREENS - SAME MODE       |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->nfs, mA   | sb==cb, cb!=NULL                     |

| USE CASE: SWITCHING BETWEEN FULL SCREEN SCREENS - DIFFERENT MODES                         |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->fs, mB    | sb==ntb, cb==NULL                    |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->fs, mB    | disp(cb), sb==ntb, cb==NULL          |
| USE CASE: SWITCHING BETWEEN FULL SCREEN AND NOT FULL SCREEN SCREENS - DIFFERENT MODES     |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->nfs, mB   | new(cb), sb==cb, cb!=NULL            |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->nfs, mB   | disp(cb), new(cb), sb==cb, cb!=NULL  |
| USE CASE: SWITCHING BETWEEN NOT FULL SCREEN AND FULL SCREEN SCREENS - DIFFERENT MODES     |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->fs, mB    | disp(cb), sb==ntb, cb==NULL          |
| USE CASE: SWITCHING BETWEEN NOT FULL SCREEN AND NOT FULL SCREEN SCREENS - DIFFERENT MODES |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->nfs, mB   | disp(cb), new(cb), sb==cb, cb!=NULL  |

| USE CASE: DRAGGING SCREEN DOWN                                                            |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->nfs, mA   | new(cb), sb==cb                      |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->nfs, mA   | sb==cb                               |
| USE CASE: DRAGGING SCREEN UP                                                              |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->fs, mA    | sb==etb                              |

Resulting rules (order matters):

(a) if ((cb!=NULL) && (etb->mode!=ntb->mode)) {dispose(cb), cb=NULL}
(b) if ((ntb->nfs) && (cb==NULL)) new(cb)
(c) if (ntb->nfs) sb=cb
(d) if (ntb->fs) sb=ntb

Additional rule:
(e) if (oldsb!=sb) modeswitch(sb)

We don't remember sb because we don't handle it in any way. Instead we either
have or don't have displaybitmap. If we have it, composition is on (sb = cb).
If we don't have it, composition is off (sb = ntb).
*/

/*
 * Does a screen need the composed bitmap for its surfaces? Composited layers
 * only exist there, and so does a client surface the compositor blits (RGB
 * without a plane). One on a plane does not (the plane sits above a
 * passthrough primary just as well, the common video-on-Workbench case), nor
 * does a video surface that is off: nothing of it is drawn either way.
 */
static BOOL HIDDCompositorNodeNeedsComposition(struct HIDDCompositorData *compdata, struct StackBitMapNode *n)
{
    struct SurfaceNode *sn;

    if (n->haslayersurfaces)
        return TRUE;
    ForeachNode(&n->surfaces, sn)
    {
        if (sn->client && (sn->plane < 0) && SURFACE_BLITTABLE(sn))
            return TRUE;
    }
    return FALSE;
}

/* Must the display be composed, 'topnode' being the frontmost screen that is not on a plane? */
static BOOL HIDDCompositorNeedsComposition(struct HIDDCompositorData *compdata, struct StackBitMapNode *topnode)
{
    struct Rectangle toprect;

    HIDDCompositorNodeRect(topnode, &toprect);
    if ((toprect.MinY > 0) || ((compdata->displayrect.MaxY - compdata->displayrect.MinY + 1) > topnode->dsth))
        return TRUE;
    if ((toprect.MinX > 0) || ((compdata->displayrect.MaxX - compdata->displayrect.MinX + 1) > topnode->dstw))
        return TRUE;
    if (topnode->sbmflags & COMPF_ALPHA)
        return TRUE;
    if (HIDDCompositorNodeNeedsComposition(compdata, topnode))
        return TRUE;
    if (topnode->scaled)
        return TRUE;    /* passthrough shows the bitmap as it is: a scaled screen must be composed */
    return FALSE;
}

/* The frontmost screen that is not on a plane (the composed bitmap starts there), NULL when all are */
static struct StackBitMapNode *HIDDCompositorFirstComposedNode(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode *topnode = (struct StackBitMapNode *)compdata->bitmapstack.mlh_Head;

    while (topnode->n.mln_Succ && (topnode->plane >= 0))
        topnode = (struct StackBitMapNode *)topnode->n.mln_Succ;
    return topnode->n.mln_Succ ? topnode : NULL;
}

static BOOL HIDDCompositorToggleCompositing(struct HIDDCompositorData *compdata, BOOL newtop)
{
    /*
     * If the topbitmap covers the complete screen, show it instead of
     * displaybitmap. Remember that screen bitmap -> composited bitmap
     * mirroring has a negative impact on performance.
     */
    OOP_Object *olddisplaybitmap = compdata->displaybitmap;
    OOP_Object *oldintermedbitmap = compdata->intermedbitmap;
    struct StackBitMapNode *topnode = (struct StackBitMapNode *)compdata->bitmapstack.mlh_Head;
    OOP_Object *newsdispbitmap = NULL;
    OOP_Object *fb = NULL;

    BOOL ok = TRUE, composit = FALSE;

    OOP_GetAttr(compdata->display, aHidd_Display_FrameBuffer, (IPTR *)&fb);

    /* Screens on planes are above the composed bitmap; it starts at the first one that is not */
    while (topnode->n.mln_Succ && (topnode->plane >= 0))
        topnode = (struct StackBitMapNode *)topnode->n.mln_Succ;
    if (!topnode->n.mln_Succ)
    {
        /*
         * Every screen is on a plane: the primary shows nothing of its own, but
         * it still carries the display timing, so it follows the stack's mode
         * like in the composed case, and is blanked where no plane covers it.
         */
        if (fb)
        {
            if (compdata->displaybitmap && (compdata->displaybitmap != fb))
                HIDDCompositorFreeBitMap(compdata, compdata->displaybitmap);
            compdata->displaybitmap = NULL;
            if (compdata->modeschanged)
                OOP_SetAttrsTags(fb, aHidd_BitMap_ModeID, compdata->displaymode, TAG_DONE);
            compdata->screenbitmap = HIDD_Display_Show(compdata->display, fb, 0);
            HIDD_BM_FillRect(fb, compdata->gc, 0, 0, compdata->displayrect.MaxX, compdata->displayrect.MaxY);
        }
        else
        {
            /*
             * No framebuffer: the primary scans a bitmap of ours out directly,
             * so it needs a blank one in the display's mode under the planes
             * (showing NULL would leave the crtc on whatever it showed before).
             */
            if (compdata->displaybitmap && compdata->modeschanged)
            {
                HIDDCompositorFreeBitMap(compdata, compdata->displaybitmap);
                compdata->displaybitmap = NULL;
            }
            if (!compdata->displaybitmap)
                compdata->displaybitmap = HIDDCompositorAllocBitMap(compdata, BMF_DISPLAYABLE);
            if (compdata->displaybitmap)
            {
                HIDD_BM_FillRect(compdata->displaybitmap, compdata->gc, 0, 0, compdata->displayrect.MaxX, compdata->displayrect.MaxY);
                compdata->screenbitmap = HIDD_Display_Show(compdata->display, compdata->displaybitmap, 0);
            }
            else
                compdata->screenbitmap = HIDD_Display_Show(compdata->display, NULL, 0);
        }
        compdata->passthroughbm = NULL;
        compdata->modeschanged = FALSE;
        return TRUE;
    }

    /* (a) If mode change is needed, enforce opening a new screen */
    if (compdata->modeschanged)
    {
        DTOGGLE(bug("[Compositor] %s: Display Mode changed\n", __func__));
        compdata->displaybitmap = NULL;
    }

    composit = HIDDCompositorNeedsComposition(compdata, topnode);

    if (composit)
    {
        /* (b) */
        if (compdata->displaybitmap == NULL)
        {
            /*
             * displaybitmap == NULL means we were in passthrough mode before,
             * or have just changed display mode - set up screen for composition.
             */
            DTOGGLE(bug("[Compositor] %s: Initialising Display-Compositor..\n", __func__));

            if (fb)
            {
                /*
                 * If our display driver uses a framebuffer, we can reuse it.
                 * Copy its original contents back into the bitmap which it replaced,
                 * then change framebuffer's video mode.
                 * Framebuffer is the only bitmap which can change its ModeID on the fly.
                 */
                DTOGGLE(bug("[Compositor] %s: Using Display Famebuffer HiddBitMap @ 0x%p\n", __func__, fb));

                /* Do this comparison in order not to show the framebuffer twice */
                if (olddisplaybitmap != fb)
                {
                    /*
                     * 1. It's legal to show the framebuffer itself. This causes copying
                     *    back old bitmap contents and detaching from it.
                     * 2. The result of this will always match fb.
                     * 3. Internally this is a simple blit operation, it can't fail.
                     */
                    compdata->screenbitmap = HIDD_Display_Show(compdata->display, fb, fHidd_Display_Show_CopyBack);
                }

                /* Switch display mode on the framebuffer. */
                OOP_SetAttrsTags(fb, aHidd_BitMap_ModeID, compdata->displaymode, TAG_DONE);
                /* We are now compositing on the framebuffer */
                compdata->displaybitmap = fb;
            }
            else
            {
                /*
                 * There's no framebuffer.
                 * Create a new bitmap that will be used for compositing.
                 */
                compdata->displaybitmap = HIDDCompositorAllocBitMap(compdata, BMF_DISPLAYABLE);
                if (compdata->displaybitmap)
                {
                    DTOGGLE(bug("[Compositor] %s: Created Compositor Display BitMap @ 0x%p\n", __func__, compdata->displaybitmap));

                    /* Mode changed, this bitmap will be shown later */
                    newsdispbitmap = compdata->displaybitmap;
                }
                else
                    ok = FALSE;
            }
        }
        else /* if (compdata->displaybitmap == NULL) */
        {
            /*
             * We are already in compositing mode and will stay in it.
             * Do not destroy our working bitmap.
             */
            olddisplaybitmap = NULL;
        }

        if ((compdata->flags & COMPSTATEF_HASALPHA) && !(compdata->intermedbitmap))
        {
            compdata->intermedbitmap = HIDDCompositorAllocBitMap(compdata, 0);
            DTOGGLE(bug("[Compositor] %s: Allocated Alpha Intermediary BitMap @ 0x%p\n", __func__, compdata->intermedbitmap));
        }
        else if (!(compdata->flags & COMPSTATEF_HASALPHA) && (compdata->intermedbitmap))
            compdata->intermedbitmap = NULL;

        /*
         * (c) Here composition is turned on (displaybitmap != NULL).
         * Redraw bitmap stack - compensate possible changes
         */
        if (ok)
            HIDDCompositorRedrawVisibleRegions(compdata, NULL);
    }
    else if (olddisplaybitmap || newtop || (topnode->bm != compdata->passthroughbm))
    {
        /*
         * (d) Set passthrough mode and display the frontmost bitmap.
         * This is also triggered by 'newtop' parameter, which tells us
         * that frontmost bitmap has been changed, and we need to display a new one.
         * With planes the shown bitmap is the frontmost one *without* a plane,
         * which can change while the frontmost bitmap stays the same (and the
         * previously shown one may be about to be freed), hence the last test.
         * Old displaybitmap has been remembered in the beginning. If it's not
         * NULL, it will be destroyed in the end.
         */
        newsdispbitmap = topnode->bm;
        compdata->displaybitmap = NULL;
    }
    compdata->passthroughbm = compdata->displaybitmap ? NULL : topnode->bm;

    DTOGGLE(bug("[Compositor] %s: oldcompbm 0x%p, topbm 0x%p, dispbm 0x%p, newdispbm 0x%p\n", __func__,
            olddisplaybitmap, compdata->topbitmap, compdata->displaybitmap, newsdispbitmap));

    /*
     * (e) If the screenbitmap changed, show the new screenbitmap.
     * We do it after refreshing, for better visual appearance.
     */
    if (newsdispbitmap)
    {
        IPTR w, h;

        compdata->screenbitmap = HIDD_Display_Show(compdata->display, newsdispbitmap, fHidd_Display_Show_CopyBack);
        DTOGGLE(bug("[Compositor] %s: Displayed HiddBitMap 0x%p, Show returned 0x%p\n", __func__, newsdispbitmap, compdata->screenbitmap));

        /* After Show we need Update for mirroring drivers */
        if (compdata->screenbitmap)
        {
            OOP_GetAttr(compdata->screenbitmap, aHidd_BitMap_Width, &w);
            OOP_GetAttr(compdata->screenbitmap, aHidd_BitMap_Height, &h);
            HIDD_BM_UpdateRect(compdata->screenbitmap, 0, 0, w, h);
        }
    }

    /*
     * (a) - disposing of previous compositing buffers needs to happen after mode switch
     * since it could have been the current screenbitmap
     */
    if (!(compdata->flags & COMPSTATEF_HASALPHA) && (oldintermedbitmap))
    {
        DTOGGLE(bug("[Compositor] %s: Disposing old alpha-intermediate bitmap 0x%p\n", __func__, oldintermedbitmap));
        HIDDCompositorFreeBitMap(compdata, oldintermedbitmap);
        compdata->intermedbitmap = NULL;
    }

    if (olddisplaybitmap && (olddisplaybitmap != fb))
    {
        DTOGGLE(bug("[Compositor] %s: Disposing old display bitmap 0x%p\n", __func__, olddisplaybitmap));
        HIDDCompositorFreeBitMap(compdata, olddisplaybitmap);
    }

    /* Handled */
    compdata->modeschanged = FALSE;

    return ok;
}

/*
 * The plane demand changed (a client surface came, went or moved): plan the
 * planes again, switch composition on or off as the result asks, redraw
 * 'dirty' (display relative, may be NULL) where composing, commit, stamp. A
 * full redraw only happens when a screen changed planes or composition
 * toggles. 'cs' (may be NULL) has its new region added to 'dirty'.
 */
static void HIDDCompositorReplan(struct HIDDCompositorData *compdata, struct ClientSurface *cs,
                                 struct Region *dirty, struct HookFires *fires)
{
    struct StackBitMapNode *n, *topnode;
    BOOL changed = FALSE;

    if (IsListEmpty(&compdata->bitmapstack))
    {
        HIDDCompositorQueuePlaneHooks(compdata, fires);
        return;
    }

    ForeachNode(&compdata->bitmapstack, n)
        n->lastplane = n->plane;
    HIDDCompositorPlanPlanes(compdata);
    if ((compdata->planecount < 2) && !compdata->nclientsurfaces)
        HIDDCompositorRecalculateVisibleRegions(compdata);      /* AssignPlanes() did not */
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->plane != n->lastplane)
            changed = TRUE;
    }
    if (cs && cs->sn && cs->sn->region && dirty)
        OrRegionRegion(cs->sn->region, dirty);

    topnode = HIDDCompositorFirstComposedNode(compdata);
    if (changed || (topnode && (HIDDCompositorNeedsComposition(compdata, topnode) != (compdata->displaybitmap != NULL))))
        HIDDCompositorToggleCompositing(compdata, FALSE);       /* redraws everything when composing */
    else if (compdata->displaybitmap)
    {
        if (dirty)
        {
            struct RegionRectangle *rr;

            for (rr = dirty->RegionRectangle; rr; rr = rr->Next)
            {
                struct Rectangle r;

                r.MinX = rr->bounds.MinX + dirty->bounds.MinX;
                r.MinY = rr->bounds.MinY + dirty->bounds.MinY;
                r.MaxX = rr->bounds.MaxX + dirty->bounds.MinX;
                r.MaxY = rr->bounds.MaxY + dirty->bounds.MinY;
                HIDDCompositorRedrawVisibleRegions(compdata, &r);
            }
        }
        else
            HIDDCompositorRedrawVisibleRegions(compdata, NULL);
    }
    HIDDCompositorCommitPlanes(compdata, 0, fires);
    HIDDCompositorStampKeys(compdata);
}

static VOID HIDDCompositorPurgeBitMapStack(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode *curr, *next;

    ForeachNodeSafe(&compdata->bitmapstack, curr, next)
    {
        HIDDCompositorFreeSurfaces(compdata, curr);
        if (curr->paintregion)
            DisposeRegion(curr->paintregion);
        if (curr->screenregion)
            DisposeRegion(curr->screenregion);

        FreeMem(curr, sizeof(struct StackBitMapNode));
    }

    NEWLIST(&compdata->bitmapstack);
}

static void HIDDCompositorShowSingle(struct HIDDCompositorData *compdata, OOP_Object *bm)
{
    OOP_Object *fb = NULL;

    OOP_GetAttr(compdata->display, aHidd_Display_FrameBuffer, (IPTR *)&fb);

    /* Show the single top bitmap */
    compdata->topbitmap = bm;
    compdata->passthroughbm = bm;
    compdata->screenbitmap = HIDD_Display_Show(compdata->display, bm, fHidd_Display_Show_CopyBack);

    /* Dispose working bitmap (if any) */
    if (compdata->displaybitmap)
    {
        /* Be careful with the framebuffer */
        if (compdata->displaybitmap != fb)
            HIDDCompositorFreeBitMap(compdata, compdata->displaybitmap);

        /* This will deactivate us */
        compdata->displaybitmap = NULL;
    }
}

/* Emergency error recovery function */
static void HIDDCompositorReset(struct HIDDCompositorData *compdata)
{
    /* Purge bitmap stack */
    HIDDCompositorPurgeBitMapStack(compdata);

    /*
     * Reset our internal state so that next BitMapStackChanged
     * causes complete reinitialization.
     */
    compdata->displaymode = vHidd_ModeID_Invalid;
    compdata->screenbitmap = NULL;
    compdata->passthroughbm = NULL;

    compdata->flags &= ~COMPSTATEF_HASALPHA;
}

AROS_UFH3(static void, CompositorDefaultBackFillFunc,
    AROS_UFHA(struct Hook *             , h,      A0),
    AROS_UFHA(struct BitMap *           , bm,     A2),
    AROS_UFHA(struct HIDD_BackFillHookMsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct HIDDCompositorData *compdata = h->h_Data;

    HIDDCompositorFillRect(compdata, HIDD_BM_OBJ(bm), msg->bounds->MinX, msg->bounds->MinY, msg->bounds->MaxX, msg->bounds->MaxY);

    AROS_USERFUNC_EXIT
}

/* Whether the display has a truecolor mode at all: composition needs one to render into */
static BOOL HIDDCompositorCanCompose(struct HIDDCompositorData *compdata)
{
    HIDDT_ModeID mode = vHidd_ModeID_Invalid;
    OOP_Object *sync, *pf;

    if (!compdata->dmenum)
        return FALSE;

    while ((mode = HIDD_DMEnum_NextModeID(compdata->dmenum, mode, &sync, &pf)) != vHidd_ModeID_Invalid)
    {
        if (pf && (OOP_GET(pf, aHidd_PixFmt_ColorModel) == vHidd_ColorModel_TrueColor))
            return TRUE;
    }
    return FALSE;
}

/****************************************************************************************/
/* Root interface                                                                       */
/****************************************************************************************/

OOP_Object *Compositor__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    /*
     * The library-base macros above reach the module data through
     * compdata->compcsd. The object (and so its instance data) only exists
     * once the superclass has allocated it, so until then a stack copy
     * carrying just the module data pointer stands in for it.
     */
    struct HIDDCompositorData stub = { .compcsd = CSD(cl) };
    struct HIDDCompositorData *compdata = &stub;

    D(bug("[Compositor] %s()\n", __func__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if (o)
    {
        struct TagItem gctags[] =
        {
            { aHidd_GC_Foreground, 0 },
            { TAG_DONE           , 0 }
        };

        compdata = OOP_INST_DATA(cl, o);
        compdata->compcsd = CSD(cl);


        compdata->display = (OOP_Object *)GetTagData(aHidd_Compositor_DisplayHidd, 0, msg->attrList);
        if (compdata->display)
        {
            OOP_GetAttr(compdata->display, aHidd_Display_GfxHidd, (IPTR *)&compdata->gfx);
            OOP_GetAttr(compdata->display, aHidd_Display_DMEnumerator, (IPTR *)&compdata->dmenum);
        }

        /* Default: AmigaOS-like dragging, until the prefs variable says otherwise */
        compdata->capabilities = (ULONG)GetTagData(aHidd_Compositor_State, COMPF_ABOVE, msg->attrList);
        compdata->modepolicy = (ULONG)GetTagData(aHidd_Compositor_ModePolicy, vHidd_Compositor_ModePolicy_FollowTop, msg->attrList);
        compdata->flags |= COMPSTATEF_DEEPLUT;
        compdata->displaymode = vHidd_ModeID_Invalid;

        NEWLIST(&compdata->bitmapstack);
        NEWLIST(&compdata->clientsurfaces);
        InitSemaphore(&compdata->semaphore);
        InitSemaphore(&compdata->hooksem);

        compdata->defaultbackfill.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(CompositorDefaultBackFillFunc);
        compdata->defaultbackfill.h_Data = compdata;
        compdata->backfillhook = (struct Hook *)GetTagData(aHidd_Compositor_BackFillHook, (IPTR)&compdata->defaultbackfill, msg->attrList);

        compdata->can_compose = HIDDCompositorCanCompose(compdata);

        /* GC object that will be used for drawing operations */
        compdata->gc = OOP_NewObject(CSD(cl)->gcclass, NULL, gctags);

        D(bug("[Compositor] %s: compositor 0x%p for display 0x%p (gfx 0x%p), can compose %d\n", __func__,
              o, compdata->display, compdata->gfx, compdata->can_compose));

        if (compdata->display && compdata->gfx && compdata->gc)
            return o;

        /* Creation failed */
        {
            OOP_MethodID disposemid = msg->mID - moRoot_New + moRoot_Dispose;
            OOP_CoerceMethod(cl, o, &disposemid);
        }
    }

    return NULL;
}

void Compositor__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);

    D(bug("[Compositor] %s: HIDDCompositorData @ 0x%p\n", __func__, compdata));

    if (compdata->GraphicsBase)
    {
        if (compdata->displaybitmap)
        {
            OOP_Object *fb = NULL;

            OOP_GetAttr(compdata->display, aHidd_Display_FrameBuffer, (IPTR *)&fb);
            if (compdata->displaybitmap != fb)
                HIDDCompositorFreeBitMap(compdata, compdata->displaybitmap);
        }
        if (compdata->intermedbitmap)
            HIDDCompositorFreeBitMap(compdata, compdata->intermedbitmap);
        if (compdata->alpharegion)
            DisposeRegion(compdata->alpharegion);
        HIDDCompositorPurgeBitMapStack(compdata);
        CloseLibrary((struct Library *)compdata->GraphicsBase);
    }
    {
        /* Client surfaces still attached (the client should have detached them) */
        struct ClientSurface *cs, *next;

        ForeachNodeSafe(&compdata->clientsurfaces, cs, next)
            FreeMem(cs, sizeof(struct ClientSurface));
    }
    if (compdata->dosbase)
        CloseLibrary(compdata->dosbase);
    if (compdata->planes)
        FreeMem(compdata->planes, compdata->planealloc * (sizeof(struct HIDD_PlaneInfo) + 2 * sizeof(struct HIDD_PlaneState)));
    if (compdata->scalesrc)
        FreeVec(compdata->scalesrc);
    if (compdata->scaledst)
        FreeVec(compdata->scaledst);
    if (compdata->scalemap)
        FreeVec(compdata->scalemap);
    if (compdata->gc)
        OOP_DisposeObject(compdata->gc);

    OOP_DoSuperMethod(cl, o, msg);
}

VOID Compositor__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_COMPOSITOR_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Compositor_DisplayHidd:
                *msg->storage = (IPTR)compdata->display;
                return;
            case aoHidd_Compositor_DisplayID:
                OOP_GetAttr(compdata->display, aHidd_Display_DisplayID, msg->storage);
                return;
            case aoHidd_Compositor_Capabilities:
                comp_ReadPrefs(compdata);
                *msg->storage = compdata->can_compose ? (IPTR)CAPABILITY_FLAGS : 0;
                return;
            case aoHidd_Compositor_State:
                /* Read the prefs now: OpenScreen() asks before the first stack change */
                comp_ReadPrefs(compdata);
                *msg->storage = compdata->can_compose ? (IPTR)(compdata->capabilities & CAPABILITY_FLAGS) : 0;
                /* Screens may be scaled with the scaling policies: Intuition converts its coordinates */
                if (compdata->can_compose && (compdata->modepolicy != vHidd_Compositor_ModePolicy_FollowTop))
                    *msg->storage |= COMPF_SCALE;
                return;
            case aoHidd_Compositor_BackFillHook:
                *msg->storage = (IPTR)compdata->backfillhook;
                return;
            case aoHidd_Compositor_FrameBuffer:
                OOP_GetAttr(compdata->display, aHidd_Display_FrameBuffer, msg->storage);
                return;
            case aoHidd_Compositor_ModePolicy:
                *msg->storage = compdata->modepolicy;
                return;
            case aoHidd_Compositor_Active:
                *msg->storage = compdata->displaybitmap ? TRUE : FALSE;
                return;
            case aoHidd_Compositor_ComposedBitMap:
                *msg->storage = (IPTR)compdata->displaybitmap;
                return;
        }
    }
    OOP_DoSuperMethod(cl, o, &msg->mID);
}

VOID Compositor__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct TagItem *tag, *tstate = msg->attrList;
    ULONG idx;

    while ((tag = NextTagItem(&tstate)))
    {
        if (IS_COMPOSITOR_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
                case aoHidd_Compositor_State:
                    D(bug("[Compositor] %s: State %lx -> %lx\n", __func__, compdata->capabilities, tag->ti_Data));
                    compdata->capabilities = (ULONG)(tag->ti_Data & CAPABILITY_FLAGS);
                    compdata->prefsread = TRUE;     /* an explicit setting overrides the prefs */
                    break;
                case aoHidd_Compositor_BackFillHook:
                    compdata->backfillhook = tag->ti_Data ? (struct Hook *)tag->ti_Data : &compdata->defaultbackfill;
                    break;
                case aoHidd_Compositor_ModePolicy:
                    compdata->modepolicy = tag->ti_Data;
                    break;
            }
        }
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/****************************************************************************************/
/* Hidd_Compositor interface                                                            */
/****************************************************************************************/

/*****************************************************************************************

    NAME
        moHidd_Compositor_BitMapStackChanged

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Compositor_BitMapStackChanged *msg);

        OOP_Object *HIDD_Compositor_BitMapStackChanged(OOP_Object *obj, struct HIDD_ViewPortData *data, BOOL *active);

    LOCATION
        hidd.gfx.compositor

    FUNCTION
        Hand the compositor a new chain of bitmaps to show, frontmost first.
        The compositor shows the frontmost bitmap directly when it covers the
        whole display, and composes otherwise.

    INPUTS
        obj    - the compositor object
        data   - the ViewPort chain of this display, or NULL for a blank display
        active - set to TRUE when composition is active (the display shows a
                 composed bitmap), FALSE for passthrough

    RESULT
        The bitmap object actually scanned out (the result of the last
        HIDD_Display_Show()), or NULL if the display is blank.

    NOTES
        Reached through HIDD_Display_ComposeViewPorts().

*****************************************************************************************/

OOP_Object *Compositor__Hidd_Compositor__BitMapStackChanged(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_BitMapStackChanged *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct HIDD_ViewPortData *vpdata;
    struct StackBitMapNode *n;
    struct HookFires fires;
    OOP_Object *bmpxfmt;
    int bmstdfmt;
    BOOL newtop = FALSE;
    BOOL ok = TRUE;

    HIDDCompositorInitFires(&fires);

    DSTACK(bug("[Compositor] %s: Top bitmap: 0x%p\n", __func__, msg->data ? msg->data->Bitmap : NULL));

    if (!comp_EnsureGraphics(compdata) || !compdata->can_compose)
    {
        /*
         * Nothing to compose with: plain passthrough of the frontmost bitmap.
         * Showing the same bitmap again is skipped, as it has always been
         * (some drivers do not like it).
         */
        OOP_Object *top = msg->data ? msg->data->Bitmap : NULL;

        if ((top != compdata->topbitmap) || !top)
        {
            compdata->topbitmap = top;
            compdata->passthroughbm = top;
            compdata->screenbitmap = HIDD_Display_Show(compdata->display, top, fHidd_Display_Show_CopyBack);
        }
        *msg->active = FALSE;
        return compdata->screenbitmap;
    }

    comp_ReadPrefs(compdata);

    LOCK_COMPOSITOR_WRITE

    /* Free all items which are already on the list */
    HIDDCompositorPurgeBitMapStack(compdata);

    if (!msg->data)
    {
        /* Blank screen */
        HIDDCompositorShowSingle(compdata, NULL);
        /* Client surfaces lost their screens' nodes, and with them their planes */
        HIDDCompositorQueuePlaneHooks(compdata, &fires);

        UNLOCK_COMPOSITOR
        HIDDCompositorFireHooks(compdata, &fires);

        /* We know we are inactive after this */
        *msg->active = FALSE;
        /* This can return NULL, it's okay */
        return compdata->screenbitmap;
    }

    /* Copy bitmaps pointers to our stack */
    for (vpdata = msg->data; vpdata; vpdata = vpdata->Next)
    {
        n = AllocMem(sizeof(struct StackBitMapNode), MEMF_ANY | MEMF_CLEAR);
        if (!n)
        {
            /*
             * Error happened.
             * We need to reset own state and return NULL. graphics.library
             * falls back to no composition in this case.
             */
            DSTACK(bug("[Compositor] %s: Error allocating StackBitMapNode!!!\n", __func__));

            ok = FALSE;
            break;
        }

        DSTACK(bug("[Compositor] %s: ViewPort 0x%p, offset (%d, %d), flags %08lx\n", __func__,
                   vpdata->vpe->ViewPort, vpdata->vpe->ViewPort->DxOffset, vpdata->vpe->ViewPort->DyOffset, vpdata->CompFlags));

        n->bm                   = vpdata->Bitmap;
        n->sbmflags             = (vpdata->CompFlags & 0xFF) | STACKNODEF_DISPLAYABLE;
        n->leftedge             = vpdata->vpe->ViewPort->DxOffset;
        n->topedge              = vpdata->vpe->ViewPort->DyOffset;
        n->prealphacomphook     = (n->sbmflags & COMPF_ALPHA) ? vpdata->PreAlphaHook : NULL;
        n->screenregion         = NewRegion();
        n->plane                = -1;
        NEWLIST(&n->surfaces);
        if (vpdata->Surfaces)
            HIDDCompositorCopySurfaces(compdata, n, vpdata->Surfaces);

        if (n->sbmflags & COMPF_ALPHA)
        {
            bmpxfmt = (OOP_Object *)OOP_GET(n->bm, aHidd_BitMap_PixFmt);
            bmstdfmt = (int)OOP_GET(bmpxfmt, aHidd_PixFmt_StdPixFmt);

            switch (bmstdfmt)
            {
                case vHidd_StdPixFmt_ARGB32:
                case vHidd_StdPixFmt_BGRA32:
                case vHidd_StdPixFmt_RGBA32:
                case vHidd_StdPixFmt_ABGR32:
                    compdata->flags |= COMPSTATEF_HASALPHA;
                    break;
                default:
                    n->sbmflags &= ~COMPF_ALPHA;
                    break;
            }
        }

        if (!(n->sbmflags & COMPF_ALPHA))
        {
            if ((((BOOL)OOP_GET(n->bm, aHidd_BitMap_Displayable)) != TRUE))
                n->sbmflags &= ~STACKNODEF_DISPLAYABLE;
        }
        AddTail((struct List *)&compdata->bitmapstack, (struct Node *)n);
    }

    /* The client surfaces of the screens that are (still, or again) on the stack */
    HIDDCompositorRelinkClientSurfaces(compdata);

    /* Switch mode if needed */
    UpdateDisplayMode(compdata);
    HIDDCompositorUpdateScaling(compdata);

    HIDDCompositorQueryPlanes(compdata);

    if (msg->data->Bitmap != compdata->topbitmap)
    {
        /* Set the new pointer to top bitmap */
        compdata->topbitmap = msg->data->Bitmap;
        newtop = TRUE;
    }

    if (ok)
    {
        /*
         * Validate bitmap offsets - they might not match the compositing rules taking
         * new displayedwidth/displayedheight values
         */
        ForeachNode(&compdata->bitmapstack, n)
        {
            HIDDCompositorValidateNodePositionChange(compdata, n, &n->leftedge, &n->topedge,
                compdata->displayrect.MaxX - compdata->displayrect.MinX + 1,
                compdata->displayrect.MaxY - compdata->displayrect.MinY + 1);
        }

        /*
         * Frontmost screens that fit a scan-out plane bypass the composed
         * bitmap, surfaces take planes too; the driver's test commit settles
         * the set before the composed area is decided.
         */
        HIDDCompositorPlanPlanes(compdata);

        /* Toogle compositing based on screen positions */
        ok = HIDDCompositorToggleCompositing(compdata, newtop);
    }

    /* Handle error */
    if (!ok)
    {
        HIDDCompositorReset(compdata);
        HIDDCompositorShowSingle(compdata, msg->data->Bitmap);
        HIDDCompositorQueuePlaneHooks(compdata, &fires);
    }
    else
    {
        HIDDCompositorCommitPlanes(compdata, 0, &fires);
        HIDDCompositorStampKeys(compdata);
    }

    UNLOCK_COMPOSITOR
    HIDDCompositorFireHooks(compdata, &fires);

    DSTACK(bug("[Compositor] %s: Done, composited bitmap 0x%p\n", __func__, compdata->displaybitmap));

    /* Tell if the composition is active (see HIDDCompositorIsActive) */
    *msg->active = HIDDCompositorIsActive(compdata);
    /* Return actually displayed bitmap */
    return compdata->screenbitmap;
}

/*****************************************************************************************

    NAME
        moHidd_Compositor_BitMapRectChanged

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Compositor_BitMapRectChanged *msg);

        VOID HIDD_Compositor_BitMapRectChanged(OOP_Object *obj, OOP_Object *bm, WORD x, WORD y, WORD width, WORD height);

    LOCATION
        hidd.gfx.compositor

    FUNCTION
        Tell the compositor that a rectangle of a bitmap has been rendered to.
        The visible part of it is re-composed onto the display. In passthrough
        the change is forwarded to the bitmap's own UpdateRect().

    NOTES
        This is the per-render-operation hot path: graphics.library reaches it
        through HIDD_Display_UpdateRect() with cached method pointers.

        Client surfaces of the screen whose Dest meets the rectangle get
        vHidd_SurfaceHook_Damage (after the lock is released). Damage to a
        client surface's own bitmap (the driver reports it on SwapBuffers of a
        bitmap that is not on a plane) re-blits the surface when it is
        blitted, and is nothing to do when a plane shows it.

*****************************************************************************************/

VOID Compositor__Hidd_Compositor__BitMapRectChanged(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_BitMapRectChanged *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    OOP_Object                *renderTarget = compdata->displaybitmap;
    struct HookFires           fires;

    HIDDCompositorInitFires(&fires);

    if (compdata->displaybitmap)
    {
        /* Composition is active, handle redraw if the bitmap is on screen */
        struct StackBitMapNode *n;

        DUPDATE(bug("[Compositor] %s: Bitmap 0x%p\n", __func__, msg->bm));

        LOCK_COMPOSITOR_READ

        if (compdata->intermedbitmap)
            renderTarget = compdata->intermedbitmap;

        n = HIDDCompositorFindBitMapStackNode(compdata, msg->bm);
        if (!n)
        {
            /* Not a screen: a surface? On a plane the driver shows the bitmap itself; a video bitmap is never blitted */
            struct StackBitMapNode *screen;
            struct SurfaceNode *sn = HIDDCompositorFindSurface(compdata, msg->bm, &screen);

            if (sn && (sn->plane < 0) && SURFACE_BLITTABLE(sn) && sn->region && (screen->sbmflags & STACKNODEF_VISIBLE))
            {
                struct HIDD_CompositeRequest *req = SNREQ(sn);
                struct Rectangle srcrect, r;
                struct RegionRectangle *rr;

                /* Surface -> screen -> display coordinates */
                {
                    LONG x0 = req->Dest.MinX + (msg->x - req->Src.MinX);
                    LONG y0 = req->Dest.MinY + (msg->y - req->Src.MinY);

                    HIDDCompositorNodeMapRect(screen, x0, y0, x0 + msg->width - 1, y0 + msg->height - 1, &srcrect);
                }

                for (rr = sn->region->RegionRectangle; rr; rr = rr->Next)
                {
                    r.MinX = rr->bounds.MinX + sn->region->bounds.MinX;
                    r.MinY = rr->bounds.MinY + sn->region->bounds.MinY;
                    r.MaxX = rr->bounds.MaxX + sn->region->bounds.MinX;
                    r.MaxY = rr->bounds.MaxY + sn->region->bounds.MinY;

                    if (AndRectRect(&srcrect, &r, &r))
                    {
                        if (sn->opaque)
                        {
                            /* Nothing shows through: blit the changed part straight in */
                            HIDDCompositorRedrawSurfaceRect(compdata, renderTarget, screen, sn, &r);
                            if (renderTarget != compdata->displaybitmap)
                                HIDD_Gfx_CopyBox(compdata->gfx, renderTarget, r.MinX, r.MinY,
                                                 compdata->displaybitmap, r.MinX, r.MinY,
                                                 r.MaxX - r.MinX + 1, r.MaxY - r.MinY + 1, compdata->gc);
                            HIDD_BM_UpdateRect(compdata->displaybitmap, r.MinX, r.MinY,
                                               r.MaxX - r.MinX + 1, r.MaxY - r.MinY + 1);
                        }
                        else
                        {
                            /* Translucent: the backdrop under it must be rebuilt first */
                            HIDDCompositorRedrawVisibleRegions(compdata, &r);
                        }
                    }
                }
            }
            UNLOCK_COMPOSITOR
            return;
        }

        /* The screen's client surfaces learn about damage inside their rectangles */
        if (n->hassurfaces)
            HIDDCompositorQueueDamage(compdata, n, msg->x, msg->y, msg->width, msg->height, &fires);

        if (n->plane >= 0)
        {
            /* Scanned out by a plane: the driver sees the change through the bitmap */
            UNLOCK_COMPOSITOR
            HIDD_BM_UpdateRect(msg->bm, msg->x, msg->y, msg->width, msg->height);
            HIDDCompositorFireHooks(compdata, &fires);
            return;
        }

        if (n->sbmflags & STACKNODEF_VISIBLE)
        {
            struct Rectangle dstandvisrect;
            struct Rectangle srcrect;
            struct RegionRectangle *srrect;
            struct Region *paint = (n->hassurfaces && n->paintregion) ? n->paintregion : n->screenregion;

            HIDDCompositorNodeMapRect(n, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1, &srcrect);

            srrect = paint->RegionRectangle;
            while (srrect)
            {
                BOOL updateAlphaBmps = FALSE;

                dstandvisrect.MinX = srrect->bounds.MinX + paint->bounds.MinX;
                dstandvisrect.MinY = srrect->bounds.MinY + paint->bounds.MinY;
                dstandvisrect.MaxX = srrect->bounds.MaxX + paint->bounds.MinX;
                dstandvisrect.MaxY = srrect->bounds.MaxY + paint->bounds.MinY;

                if (AndRectRect(&srcrect, &dstandvisrect, &dstandvisrect))
                {
                    /* Intersection is valid. Blit. */
                    if (!(n->sbmflags & COMPF_ALPHA))
                    {
                        if ((compdata->alpharegion) && (isRectInRegion(compdata->alpharegion, &dstandvisrect)))
                            updateAlphaBmps = TRUE;

                        HIDDCompositorRedrawBitmap(compdata, renderTarget, n, &dstandvisrect);
                        /* Translucent surfaces over it are blended again (opaque ones are outside 'paint') */
                        if (n->hassurfaces)
                            HIDDCompositorRedrawSurfaces(compdata, renderTarget, n, &dstandvisrect);
                    }
                    else
                    {
                        HIDDCompositorRedrawVisibleRegions(compdata, &dstandvisrect);
                    }

                    if (updateAlphaBmps)
                        HIDDCompositorRedrawAlphaRegions(compdata, &dstandvisrect);

                    if (renderTarget != compdata->displaybitmap)
                    {
                        HIDD_Gfx_CopyBox(compdata->gfx, renderTarget,
                                dstandvisrect.MinX, dstandvisrect.MinY,
                                compdata->displaybitmap,
                                dstandvisrect.MinX, dstandvisrect.MinY,
                                dstandvisrect.MaxX - dstandvisrect.MinX + 1,
                                dstandvisrect.MaxY - dstandvisrect.MinY + 1,
                                compdata->gc);
                    }
                }
                srrect = srrect->Next;
            }
            HIDD_BM_UpdateRect(compdata->displaybitmap,
                   srcrect.MinX, srcrect.MinY,
                   srcrect.MaxX - srcrect.MinX + 1,
                   srcrect.MaxY - srcrect.MinY + 1);
        }

        UNLOCK_COMPOSITOR
        HIDDCompositorFireHooks(compdata, &fires);
    }
    else
    {
        /*
         * In order to speed things up, we handle passthrough ourselves here.
         * Only a display with client surfaces attached pays for the lock
         * (the counter is a hint: a surface attached a moment later sees
         * the damage that follows).
         */
        if (compdata->nclientsurfaces)
        {
            struct StackBitMapNode *n;

            LOCK_COMPOSITOR_READ
            n = HIDDCompositorFindBitMapStackNode(compdata, msg->bm);
            if (n && n->hassurfaces)
                HIDDCompositorQueueDamage(compdata, n, msg->x, msg->y, msg->width, msg->height, &fires);
            UNLOCK_COMPOSITOR
        }
        HIDD_BM_UpdateRect(msg->bm, msg->x, msg->y, msg->width, msg->height);
        HIDDCompositorFireHooks(compdata, &fires);
    }
}

/*****************************************************************************************

    NAME
        moHidd_Compositor_BitMapPositionChange

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Compositor_BitMapPositionChange *msg);

        BOOL HIDD_Compositor_BitMapPositionChange(OOP_Object *obj, OOP_Object *bm, SIPTR *newxoffset, SIPTR *newyoffset);

    LOCATION
        hidd.gfx.compositor

    FUNCTION
        Validate and apply a new display offset of a bitmap (screen drag or
        scroll). The offsets are clamped to what the composition rules allow
        and written back.

    RESULT
        TRUE when composition is active after the move, FALSE for passthrough.

*****************************************************************************************/

BOOL Compositor__Hidd_Compositor__BitMapPositionChange(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_BitMapPositionChange *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n;
    struct HookFires fires;
    IPTR disp_width, disp_height;

    if (!compdata->GraphicsBase)
        return FALSE;

    HIDDCompositorInitFires(&fires);

    /* Regions and plane states are rewritten below: readers (BitMapRectChanged) must not see them half done */
    LOCK_COMPOSITOR_WRITE

    n = HIDDCompositorFindBitMapStackNode(compdata, msg->bm);
    if (n)
    {
        /* The bitmap is on display. Validate against screen size */
        disp_width  = compdata->displayrect.MaxX + 1;
        disp_height = compdata->displayrect.MaxY + 1;
    }
    else
    {
        /* The bitmap is not displayed yet. Validate against its own ModeID size. */
        HIDDT_ModeID modeid = vHidd_ModeID_Invalid;
        OOP_Object *bmfriend, *sync, *pf;

        OOP_GetAttr(msg->bm, aHidd_BitMap_ModeID, &modeid);

        if ((modeid == vHidd_ModeID_Invalid) && (OOP_GET(msg->bm, aHidd_BitMap_Compositable)))
        {
            OOP_GetAttr(msg->bm, aHidd_BitMap_Friend, (IPTR *)&bmfriend);
            if (bmfriend)
                OOP_GetAttr(bmfriend, aHidd_BitMap_ModeID, &modeid);
        }

        if (modeid == vHidd_ModeID_Invalid)
        {
            /*
             * Nondisplayable bitmaps don't scroll.
             * In fact they simply can't get in here because MakeVPort() performs the validation.
             * But who knows what bug can slip into someone's software...
             */
            UNLOCK_COMPOSITOR
            return FALSE;
        }

        HIDD_DMEnum_GetMode(compdata->dmenum, modeid, &sync, &pf);
        OOP_GetAttr(sync, aHidd_Sync_HDisp, &disp_width);
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &disp_height);
    }

    if (n)
        HIDDCompositorValidateNodePositionChange(compdata, n, msg->newxoffset, msg->newyoffset, disp_width, disp_height);
    else
        HIDDCompositorValidateBitMapPositionChange(compdata, msg->bm, msg->newxoffset, msg->newyoffset,
                                                    disp_width, disp_height);

    if (n && ((*msg->newxoffset != n->leftedge) || (*msg->newyoffset != n->topedge)))
    {
        struct SurfaceNode *sn;

        /* Reflect the change if it happened */
        n->leftedge = *msg->newxoffset;
        n->topedge  = *msg->newyoffset;

        /* The screen's client surfaces move with it (informational) */
        ForeachNode(&n->surfaces, sn)
        {
            if (sn->client)
                HIDDCompositorQueueHook(compdata, &fires, sn->client, vHidd_SurfaceHook_ScreenMoved, NULL, 0);
        }

        if (compdata->nclientsurfaces && (compdata->planecount >= 2))
        {
            /* Surface planes follow the visible regions (crop, key, z): plan again */
            HIDDCompositorReplan(compdata, NULL, NULL, &fires);
        }
        else if (n->plane >= 0)
        {
            /* No blit: the plane follows. Re-assign if the new position no longer fits. */
            if (HIDDCompositorPlaneFits(compdata, n->plane, n))
                HIDDCompositorCommitPlanes(compdata, fHidd_Commit_NoWait, &fires);
            else
            {
                HIDDCompositorPlanPlanes(compdata);
                HIDDCompositorToggleCompositing(compdata, TRUE);
                HIDDCompositorCommitPlanes(compdata, 0, &fires);
            }
        }
        else if (compdata->topbitmap == msg->bm)
        {
            /*
             * If this is the frontmost bitmap, we may want to toggle compositing,
             * if it starts/stops covering the whole screen at one point.
             * We don't need to call HIDDCompositorRedrawVisibleRegions() here because
             * HIDDCompositorToggleCompositing() does this itself, for improved
             * visual appearance.
             */
            HIDDCompositorToggleCompositing(compdata, FALSE);
        }
        else
            HIDDCompositorRedrawVisibleRegions(compdata, NULL);
    }

    /* Return active state (see HIDDCompositorIsActive) */
    {
        BOOL active = HIDDCompositorIsActive(compdata);
        UNLOCK_COMPOSITOR
        HIDDCompositorFireHooks(compdata, &fires);
        return active;
    }
}

/*
 * Display <-> screen pixel conversion for a screen on display (identity unless
 * the screen is scaled). Pure scaling: the screen's offset is the caller's
 * business, as Intuition subtracts LeftEdge/TopEdge itself.
 */
VOID Compositor__Hidd_Compositor__MapCoords(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_MapCoords *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n;

    LOCK_COMPOSITOR_READ
    n = HIDDCompositorFindBitMapStackNode(compdata, msg->bm);
    if (n && n->scaled)
    {
        if (msg->toDisplay)
        {
            *msg->x = NODE_X2D(n, *msg->x);
            *msg->y = NODE_Y2D(n, *msg->y);
        }
        else
        {
            *msg->x = NODE_X2S(n, *msg->x);
            *msg->y = NODE_Y2S(n, *msg->y);
        }
    }
    UNLOCK_COMPOSITOR
}

VOID Compositor__Hidd_Compositor__DisplayRectChanged(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_DisplayRectChanged *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);

    /* The display changed under us: pick the mode again at the next stack change */
    LOCK_COMPOSITOR_WRITE
    compdata->displaymode = vHidd_ModeID_Invalid;
    UNLOCK_COMPOSITOR
}

BOOL Compositor__Hidd_Compositor__BitMapValidate(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_BitMapValidate *msg)
{
    if (IS_HIDD_BM(msg->bm))
        return TRUE;

    return FALSE;
}

BOOL Compositor__Hidd_Compositor__BitMapEnable(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_BitMapEnable *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);

    if (IS_HIDD_BM(msg->bm))
    {
        if (!(OOP_GET(HIDD_BM_OBJ(msg->bm), aHidd_BitMap_Displayable)))
        {
            struct TagItem composittags[] = {
                {aHidd_BitMap_Compositable, TRUE},
                {TAG_DONE                 , 0   }
            };

            D(bug("[Compositor] %s: Marking BitMap 0x%p as Compositable\n", __func__, msg->bm));
            OOP_SetAttrs(HIDD_BM_OBJ(msg->bm), composittags);
        }
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************************

    NAME
        moHidd_Compositor_SurfaceStackChanged

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Compositor_SurfaceStackChanged *msg);

        VOID HIDD_Compositor_SurfaceStackChanged(OOP_Object *obj, struct HIDD_ViewPortData *data);

    LOCATION
        hidd.gfx.compositor

    FUNCTION
        The composited layer surfaces of one screen changed: a surface was
        added, removed, moved, resized or reordered. data->Surfaces is the
        screen's new list (back to front, screen bitmap relative); it is read
        during this call only. The display area the old and the new surfaces
        cover is re-composed.

*****************************************************************************************/

VOID Compositor__Hidd_Compositor__SurfaceStackChanged(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_SurfaceStackChanged *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n;
    struct Region *dirty;

    if (!compdata->GraphicsBase || !msg->data)
        return;

    LOCK_COMPOSITOR_WRITE

    n = HIDDCompositorFindBitMapStackNode(compdata, msg->data->Bitmap);
    if (!n)
    {
        /* Not on display: nothing to show until the next stack change */
        UNLOCK_COMPOSITOR
        return;
    }

    dirty = NewRegion();
    if (dirty)
        HIDDCompositorOrSurfaceRegions(compdata, n, dirty);

    HIDDCompositorCopySurfaces(compdata, n, msg->data->Surfaces);

    if ((n->plane >= 0) && n->haslayersurfaces)
    {
        /* A screen on a plane got composited layers: they only exist in the composed bitmap */
        struct HookFires fires;

        HIDDCompositorInitFires(&fires);
        if (dirty)
            DisposeRegion(dirty);
        HIDDCompositorReplan(compdata, NULL, NULL, &fires);
        UNLOCK_COMPOSITOR
        HIDDCompositorFireHooks(compdata, &fires);
        return;
    }

    if (!compdata->displaybitmap)
    {
        /*
         * Passthrough so far: the screen gets surfaces, which only exist in a
         * composed bitmap. Switch composition on; this redraws everything.
         */
        if (dirty)
            DisposeRegion(dirty);
        HIDDCompositorToggleCompositing(compdata, FALSE);
        UNLOCK_COMPOSITOR
        return;
    }

    HIDDCompositorRecalculateSurfaces(compdata, n);

    if (dirty)
    {
        struct RegionRectangle *rr;

        HIDDCompositorOrSurfaceRegions(compdata, n, dirty);

        /* Redraw only where a surface was or is: a move costs its old and new area */
        for (rr = dirty->RegionRectangle; rr; rr = rr->Next)
        {
            struct Rectangle r;

            r.MinX = rr->bounds.MinX + dirty->bounds.MinX;
            r.MinY = rr->bounds.MinY + dirty->bounds.MinY;
            r.MaxX = rr->bounds.MaxX + dirty->bounds.MinX;
            r.MaxY = rr->bounds.MaxY + dirty->bounds.MinY;
            HIDDCompositorRedrawVisibleRegions(compdata, &r);
        }
        DisposeRegion(dirty);
    }
    else
        HIDDCompositorRedrawVisibleRegions(compdata, NULL);

    /* With no layer surfaces left and the screen covering the display, passthrough may resume */
    if (!n->haslayersurfaces && (n == (struct StackBitMapNode *)compdata->bitmapstack.mlh_Head) &&
        !HIDDCompositorNodeNeedsComposition(compdata, n))
    {
        HIDDCompositorToggleCompositing(compdata, FALSE);
        HIDDCompositorStampKeys(compdata);
    }

    UNLOCK_COMPOSITOR
}

/*****************************************************************************************

    NAME
        moHidd_Compositor_AttachSurface

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Compositor_AttachSurface *msg);

        BOOL HIDD_Compositor_AttachSurface(OOP_Object *obj, OOP_Object *screenBitMap,
                                           struct HIDD_CompositeRequest *request, struct Hook *hook);

    LOCATION
        hidd.gfx.compositor

    FUNCTION
        Attach a client surface (a video overlay, a 3D colour buffer) to a
        screen on this display. The request is the client's and is not
        copied: Dest/Shape are in screen bitmap coordinates, like a layer
        surface's; the client changes it only through UpdateSurface. The
        surface joins the screen's surface list behind the layer surfaces and
        is shown by a scan-out plane when one fits (CRF_PLANE_OK or
        CRF_PLANE_REQUIRED and a Scanout bitmap), blitted when it is RGB and
        has none, and not at all when it is video and has none.

        The surface stays attached while its screen leaves the display and
        comes back. See the hook firing rule at the top of this file for when
        the hook runs.

    RESULT
        FALSE when the screen bitmap is not on the display's stack (the client
        may retry once the screen is shown), the request is attached already,
        or memory is short.

    SEE ALSO
        moHidd_Display_AttachSurface

*****************************************************************************************/

BOOL Compositor__Hidd_Compositor__AttachSurface(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_AttachSurface *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n;
    struct ClientSurface *cs;
    struct HookFires fires;
    struct Region *dirty;

    if (!compdata->GraphicsBase || !msg->request || !msg->screenBitMap)
        return FALSE;

    HIDDCompositorInitFires(&fires);

    LOCK_COMPOSITOR_WRITE

    n = HIDDCompositorFindBitMapStackNode(compdata, msg->screenBitMap);
    if (!n || HIDDCompositorFindClientSurface(compdata, msg->request))
    {
        UNLOCK_COMPOSITOR
        return FALSE;
    }

    cs = AllocMem(sizeof(struct ClientSurface), MEMF_ANY | MEMF_CLEAR);
    if (!cs)
    {
        UNLOCK_COMPOSITOR
        return FALSE;
    }
    cs->screenbm  = msg->screenBitMap;
    cs->req       = msg->request;
    cs->hook      = msg->hook;
    cs->hookplane = -1;
    if (!HIDDCompositorLinkClientSurface(compdata, n, cs))
    {
        FreeMem(cs, sizeof(struct ClientSurface));
        UNLOCK_COMPOSITOR
        return FALSE;
    }
    AddTail((struct List *)&compdata->clientsurfaces, (struct Node *)cs);
    compdata->nclientsurfaces++;

    bug("[Compositor] AttachSurface: request 0x%p (bitmap 0x%p, scanout %d video %lu, flags 0x%x) on screen 0x%p\n",
        msg->request, msg->request->BitMap, cs->sn ? cs->sn->scanout : -1, cs->sn ? (unsigned long)cs->sn->videofmt : 0UL,
        msg->request->Flags, msg->screenBitMap);

    HIDDCompositorQueryPlanes(compdata);
    dirty = NewRegion();
    HIDDCompositorReplan(compdata, cs, dirty, &fires);
    if (dirty)
        DisposeRegion(dirty);

    UNLOCK_COMPOSITOR
    HIDDCompositorFireHooks(compdata, &fires);

    return TRUE;
}

/*****************************************************************************************

    NAME
        moHidd_Compositor_UpdateSurface

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Compositor_UpdateSurface *msg);

        VOID HIDD_Compositor_UpdateSurface(OOP_Object *obj, struct HIDD_CompositeRequest *request);

    LOCATION
        hidd.gfx.compositor

    FUNCTION
        The client changed its attached request (Dest, Shape, Src, flags,
        bitmap). The surface's visible region, plane, crop and colour key are
        worked out again; the display area it covered and covers is redrawn
        and its plane re-committed. May be called from the surface hook.

*****************************************************************************************/

VOID Compositor__Hidd_Compositor__UpdateSurface(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_UpdateSurface *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct ClientSurface *cs;
    struct HookFires fires;

    if (!compdata->GraphicsBase || !msg->request)
        return;

    HIDDCompositorInitFires(&fires);

    LOCK_COMPOSITOR_WRITE

    cs = HIDDCompositorFindClientSurface(compdata, msg->request);
    if (cs && cs->sn)
    {
        struct Region *dirty = NewRegion();

        if (dirty && cs->sn->region)
            OrRegionRegion(cs->sn->region, dirty);
        HIDDCompositorSurfaceBitMapAttrs(compdata, cs->sn);     /* the bitmap may be another one */
        HIDDCompositorReplan(compdata, cs, dirty, &fires);
        if (dirty)
            DisposeRegion(dirty);
    }
    /* else: not attached, or its screen is not on the display: nothing is shown of it */

    UNLOCK_COMPOSITOR
    HIDDCompositorFireHooks(compdata, &fires);
}

/*****************************************************************************************

    NAME
        moHidd_Compositor_DetachSurface

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Compositor_DetachSurface *msg);

        VOID HIDD_Compositor_DetachSurface(OOP_Object *obj, struct HIDD_CompositeRequest *request);

    LOCATION
        hidd.gfx.compositor

    FUNCTION
        Detach a client surface: its plane is switched off, the display area
        it covered is redrawn, and the request and hook are not referred to
        again once this returns. No PlaneLost is sent for the detaching
        surface. Must not be called from the surface hook (it waits for the
        hook calls in flight to return; see the hook firing rule).

*****************************************************************************************/

VOID Compositor__Hidd_Compositor__DetachSurface(OOP_Class *cl, OOP_Object *o, struct pHidd_Compositor_DetachSurface *msg)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct ClientSurface *cs;
    struct HookFires fires;
    ULONG pending;

    if (!msg->request)
        return;

    HIDDCompositorInitFires(&fires);

    LOCK_COMPOSITOR_WRITE

    cs = HIDDCompositorFindClientSurface(compdata, msg->request);
    if (!cs)
    {
        UNLOCK_COMPOSITOR
        return;
    }

    D(bug("[Compositor] %s: request 0x%p\n", __func__, msg->request));

    if (cs->sn)
    {
        struct StackBitMapNode *n = HIDDCompositorFindBitMapStackNode(compdata, cs->screenbm);
        struct Region *dirty = NewRegion();

        if (dirty && cs->sn->region)
            OrRegionRegion(cs->sn->region, dirty);
        if (n)
            HIDDCompositorUnlinkClientSurface(compdata, n, cs->sn);
        else
            HIDDCompositorFreeSurfaceNode(compdata, cs->sn);     /* cannot happen: a node implies its screen */
        Remove((struct Node *)cs);
        compdata->nclientsurfaces--;
        if (compdata->GraphicsBase)
            HIDDCompositorReplan(compdata, NULL, dirty, &fires);
        if (dirty)
            DisposeRegion(dirty);
    }
    else
    {
        Remove((struct Node *)cs);
        compdata->nclientsurfaces--;
    }

    /*
     * Unlinked: no further call can be collected for it. Register as the
     * waiter before the lock goes (the signal is cleared first so that a
     * call returning between here and Wait() is not missed), then wait for
     * the calls other tasks collected before the lock was taken.
     */
    SetSignal(0, SIGF_SINGLE);
    ObtainSemaphore(&compdata->hooksem);
    cs->waiter = FindTask(NULL);
    pending = cs->firing;
    ReleaseSemaphore(&compdata->hooksem);

    UNLOCK_COMPOSITOR
    HIDDCompositorFireHooks(compdata, &fires);
    if (pending)
        Wait(SIGF_SINGLE);

    FreeMem(cs, sizeof(struct ClientSurface));
}
