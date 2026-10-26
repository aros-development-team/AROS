#ifndef AROS_SURFACETRACK_H
#define AROS_SURFACETRACK_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Surface tracking helper (static link library libsurfacetrack.a).

    A client library that attaches a scan-out surface to a window
    (cgxvideo.library for video, gallium.library for GL) keeps the
    surface's destination rectangle and visible shape current itself;
    layers.library is not extended for it. This helper does the shared
    part: with the window's layer locked it turns a layer relative
    rectangle into the HIDD_CompositeRequest Dest (screen bitmap
    coordinates) and Shape (the visible part of Dest, NULL when all of
    it is visible), remembers the last result, and provides the
    non-blocking damage hook helper and the SNOTIFY_BEFORE_CLOSEWINDOW
    safety net.

    Uses graphics.library and intuition.library through their proto
    headers (autoinit); timer.device only through SurfaceTrackCtx.
*/

#include <exec/types.h>
#include <exec/semaphores.h>
#include <devices/timer.h>
#include <graphics/gfx.h>
#include <graphics/regions.h>
#include <intuition/intuition.h>
#include <utility/hooks.h>

struct HIDD_SurfaceHookMsg;

/*
 * Shared by all trackers of one client library. TimerBase is an opened
 * timer.device (any unit) used for ReadEClock(); NULL disables the time
 * based rate limit of st_HookDamage(). MinInterval is in microseconds.
 */
struct SurfaceTrackCtx
{
    struct Device *TimerBase;
    ULONG          MinInterval;
};

struct SurfaceTrack
{
    struct SignalSemaphore  Lock;        /* serialises computes (client task vs hook) */
    struct SurfaceTrackCtx *Ctx;         /* may be NULL                                */
    struct Window          *Window;
    struct Layer           *Layer;       /* Window->WLayer                             */
    struct Rectangle        Inner;       /* layer relative surface rectangle           */

    /* Last result (valid when Valid) */
    struct Rectangle        Dest;        /* screen bitmap coordinates                  */
    struct Region          *Shape;       /* NULL = all of Dest visible                 */
    struct Rectangle        LastBounds;  /* Layer->bounds at that compute              */

    struct Region          *Regions[2];  /* double buffered shape regions              */
    UBYTE                   Current;     /* index of the region behind Shape           */
    UBYTE                   pad[3];
    volatile BOOL           Stale;       /* a recompute is due                         */
    BOOL                    Changed;     /* last compute differed from the previous    */
    BOOL                    Valid;
    struct EClockVal        LastTime;    /* last hook driven recompute                 */
    ULONG                   EClockFreq;
};

/* SNOTIFY_BEFORE_CLOSEWINDOW safety net */
typedef void (*ST_CloseFunc)(struct Window *window, APTR userData);

struct SurfaceTrackNotify
{
    struct Hook  Hook;
    APTR         Handle;    /* StartScreenNotifyTagList() result */
    ST_CloseFunc Func;
    APTR         UserData;
};

/*
 * st_Init allocates the regions; st_Cleanup frees them. ctx may be NULL.
 */
BOOL st_Init(struct SurfaceTrack *st, struct SurfaceTrackCtx *ctx);
void st_Cleanup(struct SurfaceTrack *st);

/*
 * Forget the window (after a detach). The last result stays readable.
 */
void st_Reset(struct SurfaceTrack *st);

/*
 * The window's inner rectangle (inside the borders) minus the indents,
 * relative to Window->WLayer: for a GimmeZeroZero window that is the
 * inner layer, so the result starts at (0,0); otherwise it starts at
 * (BorderLeft, BorderTop). Returns FALSE when the rectangle is empty
 * (the result is then normalised to a 1x1 rectangle at its origin and
 * st_Compute() yields an empty Shape for it).
 */
BOOL st_WindowInnerRect(struct Window *window, LONG leftIndent, LONG topIndent,
                        LONG rightIndent, LONG bottomIndent, struct Rectangle *out);

/*
 * Compute Dest and Shape for innerRect (layer relative) of window.
 *
 * With mayBlock the window's layer is locked with LockLayerRom(),
 * otherwise with AttemptLockLayerRom(); when that (or the tracker's own
 * lock) cannot be taken the tracker is marked stale and FALSE is
 * returned. FALSE is also returned when a region operation runs out of
 * memory. On success Stale is cleared, destOut/shapeOut (either may be
 * NULL) receive the result and Changed is set when it differs from the
 * previous result (always for the first compute of a window).
 *
 * window may be NULL to reuse the window and rectangle of the previous
 * call; innerRect may be NULL to reuse the rectangle.
 *
 * The Shape region belongs to the tracker. Two regions alternate, so
 * after a compute that reports a change the caller must hand the new
 * Dest/Shape to the compositor (UpdateSurface) before the next compute:
 * that compute reuses the region handed out before the change.
 *
 * Visibility: the union of the layer's visible ClipRects (cr->lobs ==
 * NULL) of its regular list (l->_cliprects while a ClipRegion is
 * installed, l->cr2 while LAYERUPDATING, l->ClipRect otherwise),
 * intersected with Dest. A composited layer (LAYERCOMPOSITED) has no
 * visible ClipRects (all are views into its surface), for it the
 * visible part is visibleshape within VisibleRegion, as layers.library
 * itself reports the layer's surface. An invisible layer yields an
 * empty (non NULL) Shape.
 */
BOOL st_Compute(struct SurfaceTrack *st, struct Window *window,
                struct Rectangle *innerRect, struct Rectangle *destOut,
                struct Region **shapeOut, BOOL mayBlock);

/*
 * For the client's surface hook (runs in the damaging task; never
 * blocks): marks the tracker stale for a damage message and recomputes
 * when the tracker's lock and the layer lock can be taken without
 * waiting and the rate limit (SurfaceTrackCtx) allows. Returns TRUE when
 * a recompute happened and changed Dest/Shape (st_Changed() is then set
 * too); the caller updates its request and calls UpdateSurface. Other
 * message types (PlaneGained/PlaneLost/ScreenMoved) need no recompute
 * and return FALSE.
 */
BOOL st_HookDamage(struct SurfaceTrack *st, struct HIDD_SurfaceHookMsg *msg);

/*
 * As st_HookDamage(), but first replaces the layer relative rectangle
 * (the window may have been resized since the last client call).
 */
BOOL st_HookDamageRect(struct SurfaceTrack *st, struct HIDD_SurfaceHookMsg *msg,
                       struct Rectangle *innerRect);

/*
 * TRUE once after a compute whose Dest/Shape differ from the ones
 * before; cleared by the call.
 */
BOOL st_Changed(struct SurfaceTrack *st);

/* Stale flag access (a stale tracker needs a recompute) */
void st_SetStale(struct SurfaceTrack *st);
BOOL st_IsStale(struct SurfaceTrack *st);

/*
 * Register func to be called (in the closing task) for every window
 * about to be closed (SNOTIFY_BEFORE_CLOSEWINDOW). The client detaches
 * any surface still attached to that window. FALSE if intuition refuses.
 */
BOOL st_StartCloseNotify(struct SurfaceTrackNotify *notify, ST_CloseFunc func, APTR userData);
void st_EndCloseNotify(struct SurfaceTrackNotify *notify);

#endif /* AROS_SURFACETRACK_H */
