/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Surface tracking helper: layer lock + ClipRect walk -> Dest/Shape,
    damage hook rate limiting, SNOTIFY_BEFORE_CLOSEWINDOW safety net.
    See surfacetrack.h.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/asmcall.h>
#include <string.h>
#include <exec/memory.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <graphics/regions.h>
#include <hidd/gfx.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/timer.h>

#include "surfacetrack.h"

/*****************************************************************************/

BOOL st_Init(struct SurfaceTrack *st, struct SurfaceTrackCtx *ctx)
{
    memset(st, 0, sizeof(*st));
    InitSemaphore(&st->Lock);
    st->Ctx = ctx;

    st->Regions[0] = NewRegion();
    st->Regions[1] = NewRegion();
    if (!st->Regions[0] || !st->Regions[1])
    {
        st_Cleanup(st);
        return FALSE;
    }

    return TRUE;
}

void st_Cleanup(struct SurfaceTrack *st)
{
    if (st->Regions[0])
        DisposeRegion(st->Regions[0]);
    if (st->Regions[1])
        DisposeRegion(st->Regions[1]);
    st->Regions[0] = st->Regions[1] = NULL;
    st->Shape  = NULL;
    st->Window = NULL;
    st->Layer  = NULL;
    st->Valid  = FALSE;
}

void st_Reset(struct SurfaceTrack *st)
{
    ObtainSemaphore(&st->Lock);
    st->Window = NULL;
    st->Layer  = NULL;
    st->Stale  = FALSE;
    ReleaseSemaphore(&st->Lock);
}

void st_SetStale(struct SurfaceTrack *st)
{
    st->Stale = TRUE;
}

BOOL st_IsStale(struct SurfaceTrack *st)
{
    return st->Stale;
}

BOOL st_Changed(struct SurfaceTrack *st)
{
    BOOL changed = st->Changed;

    st->Changed = FALSE;
    return changed;
}

/*****************************************************************************/

BOOL st_WindowInnerRect(struct Window *window, LONG leftIndent, LONG topIndent,
                        LONG rightIndent, LONG bottomIndent, struct Rectangle *out)
{
    if (window->Flags & WFLG_GIMMEZEROZERO)
    {
        /* WLayer is the inner layer: window relative indents map directly */
        out->MinX = 0;
        out->MinY = 0;
        out->MaxX = window->GZZWidth - 1;
        out->MaxY = window->GZZHeight - 1;
    }
    else
    {
        out->MinX = window->BorderLeft;
        out->MinY = window->BorderTop;
        out->MaxX = window->Width  - window->BorderRight  - 1;
        out->MaxY = window->Height - window->BorderBottom - 1;
    }

    out->MinX += leftIndent;
    out->MinY += topIndent;
    out->MaxX -= rightIndent;
    out->MaxY -= bottomIndent;

    if (out->MaxX < out->MinX || out->MaxY < out->MinY)
    {
        out->MaxX = out->MinX;
        out->MaxY = out->MinY;
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

static inline BOOL st_SameRect(const struct Rectangle *a, const struct Rectangle *b)
{
    return a->MinX == b->MinX && a->MinY == b->MinY &&
           a->MaxX == b->MaxX && a->MaxY == b->MaxY;
}

static BOOL st_SameRegion(struct Region *a, struct Region *b)
{
    struct RegionRectangle *ra = a->RegionRectangle;
    struct RegionRectangle *rb = b->RegionRectangle;

    if (!ra || !rb)
        return ra == rb;    /* both empty */

    if (!st_SameRect(&a->bounds, &b->bounds))
        return FALSE;

    while (ra && rb)
    {
        if (!st_SameRect(&ra->bounds, &rb->bounds))
            return FALSE;
        ra = ra->Next;
        rb = rb->Next;
    }

    return ra == NULL && rb == NULL;
}

/* TRUE when the region is exactly the one rectangle r */
static BOOL st_RegionIsRect(struct Region *reg, const struct Rectangle *r)
{
    struct RegionRectangle *rr = reg->RegionRectangle;

    if (!rr || rr->Next)
        return FALSE;

    return st_SameRect(&reg->bounds, r);
}

/*
 * The compute proper. Called with st->Lock held. mayBlock selects the
 * layer lock call. Returns FALSE (and leaves Stale set) when the layer
 * could not be locked or a region operation failed.
 */
static BOOL st_Recompute(struct SurfaceTrack *st, BOOL mayBlock)
{
    struct Layer *l = st->Layer;
    struct Region *work;
    struct Rectangle dest, bounds;
    BOOL empty, ok = TRUE, nullshape, changed;
    UBYTE workidx;

    if (!l)
        return FALSE;

    /* The region not currently handed out as Shape */
    workidx = st->Current ^ 1;
    if (st->Shape == NULL)
        workidx = st->Current;  /* nothing handed out, either is free; keep Current stable */
    work = st->Regions[workidx];

    st->Stale = FALSE;      /* damage arriving from here on sets it again */

    /* Our own lock: damage from inside a layers operation, ClipRects not final yet */
    if (mayBlock)
        LockLayerRom(l);
    else if ((l->Lock.ss_Owner == FindTask(NULL)) || !AttemptLockLayerRom(l))
    {
        st->Stale = TRUE;
        return FALSE;
    }

    bounds = l->bounds;

    dest.MinX = st->Inner.MinX + bounds.MinX;
    dest.MinY = st->Inner.MinY + bounds.MinY;
    dest.MaxX = st->Inner.MaxX + bounds.MinX;
    dest.MaxY = st->Inner.MaxY + bounds.MinY;
    empty = (st->Inner.MaxX < st->Inner.MinX) || (st->Inner.MaxY < st->Inner.MinY);
    if (empty)
    {
        dest.MaxX = dest.MinX;
        dest.MaxY = dest.MinY;
    }

    ClearRegion(work);

    if (!empty)
    {
        if (l->Flags & LAYERCOMPOSITED)
        {
            /*
             * A composited layer has no visible ClipRects (all are views
             * into its surface); what layers.library reports as the visible
             * part of its surface is visibleshape within VisibleRegion.
             */
            if (l->visible && l->visibleshape && l->VisibleRegion)
            {
                if (!OrRegionRegion(l->visibleshape, work))
                    ok = FALSE;
                else if (!AndRegionRegion(l->VisibleRegion, work))
                    ok = FALSE;
            }
        }
        else
        {
            struct ClipRect *cr;

            /*
             * The regular ClipRect list: while a ClipRegion is installed it
             * is kept in _cliprects (ClipRect holds the clipped one), while
             * the layer is between BeginUpdate and EndUpdate it is in cr2
             * (ClipRect holds the damage list).
             */
            if (l->_cliprects)
                cr = l->_cliprects;
            else if (l->Flags & LAYERUPDATING)
                cr = l->cr2;
            else
                cr = l->ClipRect;

            for (; cr; cr = cr->Next)
            {
                if (cr->lobs == NULL)
                {
                    if (!OrRectRegion(work, &cr->bounds))
                    {
                        ok = FALSE;
                        break;
                    }
                }
            }
        }

        if (ok)
            AndRectRegion(work, &dest);
    }

    UnlockLayerRom(l);

    if (!ok)
    {
        st->Stale = TRUE;
        return FALSE;
    }

    nullshape = !empty && st_RegionIsRect(work, &dest);

    if (!st->Valid)
        changed = TRUE;
    else if (!st_SameRect(&dest, &st->Dest))
        changed = TRUE;
    else if (nullshape != (st->Shape == NULL))
        changed = TRUE;
    else if (!nullshape && !st_SameRegion(work, st->Shape))
        changed = TRUE;
    else
        changed = FALSE;

    st->LastBounds = bounds;
    st->Dest       = dest;
    st->Valid      = TRUE;

    if (changed)
    {
        st->Changed = TRUE;
        if (nullshape)
            st->Shape = NULL;
        else
        {
            st->Shape   = work;
            st->Current = workidx;
        }
    }
    /* unchanged: Shape keeps pointing at the region already handed out */

    return TRUE;
}

BOOL st_Compute(struct SurfaceTrack *st, struct Window *window,
                struct Rectangle *innerRect, struct Rectangle *destOut,
                struct Region **shapeOut, BOOL mayBlock)
{
    BOOL ok;

    if (mayBlock)
        ObtainSemaphore(&st->Lock);
    else if (!AttemptSemaphore(&st->Lock))
    {
        st->Stale = TRUE;
        return FALSE;
    }

    if (window && window != st->Window)
    {
        st->Window = window;
        st->Layer  = window->WLayer;
        st->Valid  = FALSE;
    }
    if (innerRect)
        st->Inner = *innerRect;

    ok = st_Recompute(st, mayBlock);

    if (ok)
    {
        if (destOut)
            *destOut = st->Dest;
        if (shapeOut)
            *shapeOut = st->Shape;
    }

    ReleaseSemaphore(&st->Lock);

    return ok;
}

/*****************************************************************************/

static BOOL st_RateLimited(struct SurfaceTrack *st)
{
    struct SurfaceTrackCtx *ctx = st->Ctx;
    struct Device *TimerBase;
    struct EClockVal now;
    ULONG freq, elapsed, limit;

    if (!ctx || !ctx->TimerBase || !ctx->MinInterval)
        return FALSE;

    TimerBase = ctx->TimerBase;
    freq = ReadEClock(&now);
    if (!freq)
        return FALSE;

    if (st->EClockFreq)
    {
        /* 32 bit arithmetic is enough: the intervals of interest are short */
        elapsed = now.ev_lo - st->LastTime.ev_lo;
        if (now.ev_hi != st->LastTime.ev_hi && now.ev_hi != st->LastTime.ev_hi + 1)
            elapsed = 0xffffffffUL;
        limit = (ULONG)(((UQUAD)freq * ctx->MinInterval) / 1000000);
        if (elapsed < limit)
            return TRUE;
    }

    st->EClockFreq = freq;
    st->LastTime   = now;

    return FALSE;
}

BOOL st_HookDamageRect(struct SurfaceTrack *st, struct HIDD_SurfaceHookMsg *msg,
                       struct Rectangle *innerRect)
{
    BOOL changed = FALSE;

    if (msg->Type != vHidd_SurfaceHook_Damage)
        return FALSE;

    st->Stale = TRUE;

    if (!AttemptSemaphore(&st->Lock))
        return FALSE;

    if (innerRect)
        st->Inner = *innerRect;

    if (st->Layer && !st_RateLimited(st))
    {
        if (st_Recompute(st, FALSE))
            changed = st->Changed;
    }

    ReleaseSemaphore(&st->Lock);

    return changed;
}

BOOL st_HookDamage(struct SurfaceTrack *st, struct HIDD_SurfaceHookMsg *msg)
{
    return st_HookDamageRect(st, msg, NULL);
}

/*****************************************************************************/

AROS_UFH3(static void, st_CloseNotifyHook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, object, A2),
    AROS_UFHA(struct ScreenNotifyMessage *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct SurfaceTrackNotify *notify = (struct SurfaceTrackNotify *)hook;

    /*
     * Intuition versions before the CallHookPkt() fix delivered the message
     * through the variadic CallHook(): the hook then gets a pointer to the
     * argument array whose first entry is the message. Recognise that by
     * the magic.
     */
    if (msg && (msg->snm_Message.mn_Magic != MAGIC_SCREENNOTIFY))
    {
        struct ScreenNotifyMessage *real = (struct ScreenNotifyMessage *)((IPTR *)msg)[0];

        if (real && (real->snm_Message.mn_Magic == MAGIC_SCREENNOTIFY))
            msg = real;
    }

    D(bug("[surfacetrack] close notify hook %p object %p msg %p class %lx snm_Object %p func %p\n", hook, object, msg,
          msg ? (unsigned long)msg->snm_Class : 0UL, msg ? (APTR)msg->snm_Object : NULL, notify->Func));
    if (msg && msg->snm_Class == SNOTIFY_BEFORE_CLOSEWINDOW && notify->Func)
        notify->Func((struct Window *)msg->snm_Object, notify->UserData);

    AROS_USERFUNC_EXIT
}

BOOL st_StartCloseNotify(struct SurfaceTrackNotify *notify, ST_CloseFunc func, APTR userData)
{
    struct TagItem tags[] =
    {
        { SNA_Notify,   SNOTIFY_BEFORE_CLOSEWINDOW },
        { SNA_Hook,     (IPTR)&notify->Hook        },
        { SNA_Priority, 0                          },
        { TAG_DONE,     0                          }
    };

    notify->Hook.h_Entry    = (HOOKFUNC)AROS_ASMSYMNAME(st_CloseNotifyHook);
    notify->Hook.h_SubEntry = NULL;
    notify->Hook.h_Data     = notify;
    notify->Func            = func;
    notify->UserData        = userData;
    notify->Handle          = StartScreenNotifyTagList(tags);

    return notify->Handle != NULL;
}

void st_EndCloseNotify(struct SurfaceTrackNotify *notify)
{
    if (notify->Handle)
    {
        EndScreenNotify(notify->Handle);
        notify->Handle = NULL;
    }
    notify->Func = NULL;
}
