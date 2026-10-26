/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Unit test for the surfacetrack link library: compares st_Compute()
    with the layer's visible ClipRects for a window that is moved,
    covered and uncovered, a GimmeZeroZero window and a window on a back
    screen. Exit code 0 on success, 20 on failure.
*/

#include <dos/dos.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <graphics/regions.h>
#include <hidd/gfx.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <aros/surfacetrack.h>

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond) check((cond), #cond, __LINE__)

static BOOL check(BOOL ok, const char *what, int line)
{
    if (!ok)
    {
        printf("FAIL line %d: %s\n", line, what);
        failures++;
    }
    return ok;
}

static void print_rect(const char *name, const struct Rectangle *r)
{
    printf("  %s (%d,%d)-(%d,%d)", name, r->MinX, r->MinY, r->MaxX, r->MaxY);
}

static void print_region(const char *name, struct Region *reg)
{
    struct RegionRectangle *rr;
    int n = 0;

    if (!reg)
    {
        printf("  %s NULL (all of Dest)\n", name);
        return;
    }
    for (rr = reg->RegionRectangle; rr; rr = rr->Next)
        n++;
    printf("  %s %d rect(s)", name, n);
    if (n)
        printf(" bounds (%d,%d)-(%d,%d)", reg->bounds.MinX, reg->bounds.MinY,
               reg->bounds.MaxX, reg->bounds.MaxY);
    printf("\n");
}

/*****************************************************************************
    Reference: the layer's visible ClipRects, read directly
*****************************************************************************/

static struct Region *ref_visible(struct Layer *l, const struct Rectangle *inner,
                                  struct Rectangle *dest)
{
    struct Region *r = NewRegion();
    struct ClipRect *cr;

    if (!r)
        return NULL;

    LockLayerRom(l);

    dest->MinX = inner->MinX + l->bounds.MinX;
    dest->MinY = inner->MinY + l->bounds.MinY;
    dest->MaxX = inner->MaxX + l->bounds.MinX;
    dest->MaxY = inner->MaxY + l->bounds.MinY;

    if (l->Flags & LAYERCOMPOSITED)
    {
        if (l->visible && l->visibleshape && l->VisibleRegion)
        {
            OrRegionRegion(l->visibleshape, r);
            AndRegionRegion(l->VisibleRegion, r);
        }
    }
    else
    {
        if (l->_cliprects)
            cr = l->_cliprects;
        else if (l->Flags & LAYERUPDATING)
            cr = l->cr2;
        else
            cr = l->ClipRect;
        for (; cr; cr = cr->Next)
            if (cr->lobs == NULL)
                OrRectRegion(r, &cr->bounds);
    }
    AndRectRegion(r, dest);

    UnlockLayerRom(l);

    return r;
}

/* shape NULL means the whole rectangle dest */
static BOOL region_matches(struct Region *shape, const struct Rectangle *dest, struct Region *ref)
{
    struct Region *tmp = NewRegion();
    BOOL same;

    if (!tmp)
        return FALSE;

    if (shape)
        OrRegionRegion(shape, tmp);
    else
        OrRectRegion(tmp, (struct Rectangle *)dest);
    XorRegionRegion(ref, tmp);
    same = tmp->RegionRectangle == NULL;
    DisposeRegion(tmp);

    return same;
}

static BOOL same_rect(const struct Rectangle *a, const struct Rectangle *b)
{
    return a->MinX == b->MinX && a->MinY == b->MinY && a->MaxX == b->MaxX && a->MaxY == b->MaxY;
}

/*
 * Compute with the tracker and compare with the reference. Returns the
 * shape (owned by the tracker) through shapeOut when not NULL.
 */
static BOOL compare(const char *step, struct SurfaceTrack *st, struct Window *win,
                    struct Rectangle *inner, struct Rectangle *destOut, struct Region **shapeOut)
{
    struct Rectangle dest, refdest;
    struct Region *shape = NULL, *ref;
    BOOL ok;

    printf("%s\n", step);

    ok = st_Compute(st, win, inner, &dest, &shape, TRUE);
    if (!CHECK(ok))
        return FALSE;

    ref = ref_visible(win->WLayer, inner, &refdest);
    if (!CHECK(ref != NULL))
        return FALSE;

    print_rect("Dest", &dest);
    print_rect("layer bounds", &win->WLayer->bounds);
    printf("\n");
    print_region("Shape", shape);

    ok  = CHECK(same_rect(&dest, &refdest));
    ok &= CHECK(region_matches(shape, &dest, ref));
    if (shape)
    {
        /* a Shape equal to Dest must have been reported as NULL */
        struct Region *tmp = NewRegion();

        if (tmp)
        {
            OrRectRegion(tmp, &dest);
            XorRegionRegion(shape, tmp);
            ok &= CHECK(tmp->RegionRectangle != NULL);
            DisposeRegion(tmp);
        }
    }
    ok &= CHECK(!st_IsStale(st));

    DisposeRegion(ref);

    if (destOut)
        *destOut = dest;
    if (shapeOut)
        *shapeOut = shape;

    return ok;
}

/* let input.device finish a window operation */
static void settle(void)
{
    Delay(10);
}

/*****************************************************************************/

static void test_window(struct Screen *scr)
{
    struct SurfaceTrack st;
    struct Window *win, *cover;
    struct Rectangle inner, dest, dest2, expect;
    struct Region *shape, *exp;
    struct HIDD_SurfaceHookMsg msg;
    WORD left, top;
    BOOL wasfull;

    printf("== window on the public screen ==\n");

    win = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr,
        WA_Left, 80, WA_Top, 80, WA_Width, 300, WA_Height, 200,
        WA_Title, (IPTR)"surfacetrack", WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_SimpleRefresh, TRUE, WA_Activate, TRUE, TAG_DONE);
    if (!CHECK(win != NULL))
        return;
    if (!CHECK(st_Init(&st, NULL)))
    {
        CloseWindow(win);
        return;
    }

    CHECK(st_WindowInnerRect(win, 0, 0, 0, 0, &inner));
    CHECK(inner.MinX == win->BorderLeft && inner.MinY == win->BorderTop);
    CHECK(inner.MaxX == win->Width - win->BorderRight - 1);
    CHECK(inner.MaxY == win->Height - win->BorderBottom - 1);
    CHECK(st_WindowInnerRect(win, 10, 5, 10, 5, &inner));
    CHECK(inner.MinX == win->BorderLeft + 10 && inner.MaxY == win->Height - win->BorderBottom - 6);
    CHECK(!st_WindowInnerRect(win, 1000, 0, 0, 0, &expect));
    CHECK(st_WindowInnerRect(win, 0, 0, 0, 0, &inner));

    compare("initial", &st, win, &inner, &dest, &shape);
    CHECK(st_Changed(&st));
    CHECK(!st_Changed(&st));
    CHECK(dest.MinX == win->WLayer->bounds.MinX + win->BorderLeft);

    compare("recompute, nothing changed", &st, win, &inner, &dest2, NULL);
    CHECK(!st_Changed(&st));
    CHECK(same_rect(&dest, &dest2));

    /* damage hook with nothing changed */
    memset(&msg, 0, sizeof(msg));
    msg.Type = vHidd_SurfaceHook_Damage;
    msg.Rect = dest;
    st_SetStale(&st);
    CHECK(st_IsStale(&st));
    CHECK(!st_HookDamage(&st, &msg));
    CHECK(!st_IsStale(&st));
    CHECK(!st_Changed(&st));

    left = win->LeftEdge;
    top  = win->TopEdge;
    MoveWindow(win, 37, 23);
    settle();
    CHECK(win->LeftEdge == left + 37 && win->TopEdge == top + 23);
    compare("after MoveWindow(+37,+23)", &st, win, &inner, &dest2, NULL);
    CHECK(st_Changed(&st));
    CHECK(dest2.MinX == dest.MinX + 37 && dest2.MinY == dest.MinY + 23);
    dest = dest2;

    /* damage hook after a move */
    MoveWindow(win, -17, -3);
    settle();
    CHECK(st_HookDamage(&st, &msg));
    CHECK(st_Changed(&st));
    CHECK(st.Dest.MinX == dest.MinX - 17 && st.Dest.MinY == dest.MinY - 3);
    compare("after the hook recompute", &st, win, &inner, &dest, &shape);
    CHECK(!st_Changed(&st));
    wasfull = (shape == NULL);
    if (!wasfull)
        printf("  (another window overlaps the test window; geometric check skipped)\n");

    /* a second window over part of the first */
    cover = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr,
        WA_Left, win->LeftEdge + 120, WA_Top, win->TopEdge + 90,
        WA_Width, 160, WA_Height, 140,
        WA_Title, (IPTR)"cover", WA_DragBar, TRUE, WA_Activate, TRUE, TAG_DONE);
    CHECK(cover != NULL);
    settle();

    compare("covered by a second window", &st, win, &inner, &dest2, &shape);
    CHECK(st_Changed(&st));
    CHECK(shape != NULL);
    if (cover && shape == NULL)
        printf("  (the cover window did not cover the test window?)\n");
    if (cover && shape && wasfull)
    {
        /*
         * The geometric expectation, nothing else covered the window
         * before: the shape contains Dest minus the cover's layer bounds
         * and excludes the cover's bounds inset by 8 pixels (window
         * decorations with rounded corners leave the extreme corners of
         * the cover's bounds visible).
         */
        struct Region *tmp = NewRegion();
        struct Rectangle inset = cover->WLayer->bounds;
        BOOL contains = FALSE, excludes = FALSE;

        inset.MinX += 8;
        inset.MinY += 8;
        inset.MaxX -= 8;
        inset.MaxY -= 8;

        exp = NewRegion();
        if (exp && tmp)
        {
            /* (a): (Dest - cover) - shape must be empty */
            OrRectRegion(exp, &dest2);
            ClearRectRegion(exp, &cover->WLayer->bounds);
            ClearRegionRegion(shape, exp);
            contains = exp->RegionRectangle == NULL;

            /* (b): shape within the inset cover must be empty */
            OrRegionRegion(shape, tmp);
            AndRectRegion(tmp, &inset);
            excludes = tmp->RegionRectangle == NULL;
        }
        if (!CHECK(contains && excludes))
        {
            struct RegionRectangle *rr;

            printf("  contains Dest-cover: %s, excludes inset cover: %s\n",
                   contains ? "yes" : "no", excludes ? "yes" : "no");
            printf("  cover layer bounds (%d,%d)-(%d,%d)\n", cover->WLayer->bounds.MinX, cover->WLayer->bounds.MinY,
                   cover->WLayer->bounds.MaxX, cover->WLayer->bounds.MaxY);
            for (rr = shape->RegionRectangle; rr; rr = rr->Next)
                printf("  shape rect (%d,%d)-(%d,%d)\n", rr->bounds.MinX + shape->bounds.MinX, rr->bounds.MinY + shape->bounds.MinY,
                       rr->bounds.MaxX + shape->bounds.MinX, rr->bounds.MaxY + shape->bounds.MinY);
        }
        if (exp)
            DisposeRegion(exp);
        if (tmp)
            DisposeRegion(tmp);
    }

    /* LAYERUPDATING: the regular list is in cr2 while ClipRect is the damage list */
    BeginRefresh(win);
    CHECK(win->WLayer->Flags & LAYERUPDATING);
    compare("inside BeginRefresh/EndRefresh", &st, win, &inner, &dest2, &exp);
    CHECK(!st_Changed(&st));
    EndRefresh(win, TRUE);
    CHECK(!(win->WLayer->Flags & LAYERUPDATING));

    /* move the cover away */
    if (cover)
    {
        MoveWindow(cover, 400, 300);
        settle();
        compare("cover moved away", &st, win, &inner, &dest2, &shape);
        CHECK(st_Changed(&st));
        CloseWindow(cover);
        settle();
    }

    compare("cover closed", &st, win, &inner, &dest2, &shape);

    /* indents */
    CHECK(st_WindowInnerRect(win, 8, 4, 8, 4, &inner));
    compare("with indents 8/4/8/4", &st, win, &inner, &dest2, NULL);
    CHECK(st_Changed(&st));
    CHECK(dest2.MinX == dest.MinX + 8 && dest2.MinY == dest.MinY + 4);
    CHECK(dest2.MaxX == dest.MaxX - 8 && dest2.MaxY == dest.MaxY - 4);

    /* empty rectangle: empty shape, never NULL */
    st_WindowInnerRect(win, 1000, 0, 0, 0, &inner);
    CHECK(st_Compute(&st, win, &inner, &dest2, &shape, TRUE));
    CHECK(shape != NULL && shape->RegionRectangle == NULL);

    st_Reset(&st);
    st_Cleanup(&st);
    CloseWindow(win);
}

static void test_gzz(struct Screen *scr)
{
    struct SurfaceTrack st;
    struct Window *win;
    struct Rectangle inner, dest;
    struct Region *shape;

    printf("== GimmeZeroZero window ==\n");

    win = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr,
        WA_Left, 120, WA_Top, 120, WA_Width, 260, WA_Height, 180,
        WA_Title, (IPTR)"surfacetrack GZZ", WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_GimmeZeroZero, TRUE, WA_Activate, TRUE, TAG_DONE);
    if (!CHECK(win != NULL))
        return;
    if (!CHECK(st_Init(&st, NULL)))
    {
        CloseWindow(win);
        return;
    }

    CHECK(win->Flags & WFLG_GIMMEZEROZERO);
    CHECK(win->WLayer != win->BorderRPort->Layer);
    CHECK(st_WindowInnerRect(win, 0, 0, 0, 0, &inner));
    CHECK(inner.MinX == 0 && inner.MinY == 0);
    CHECK(inner.MaxX == win->GZZWidth - 1 && inner.MaxY == win->GZZHeight - 1);

    compare("GZZ initial", &st, win, &inner, &dest, &shape);
    /* the inner layer's bounds are the inner rectangle */
    CHECK(same_rect(&dest, &win->WLayer->bounds));
    CHECK(dest.MinX == win->LeftEdge + win->BorderLeft);
    CHECK(dest.MinY == win->TopEdge + win->BorderTop);

    MoveWindow(win, 15, 25);
    settle();
    compare("GZZ moved", &st, win, &inner, &dest, &shape);
    CHECK(st_Changed(&st));
    CHECK(same_rect(&dest, &win->WLayer->bounds));

    st_Cleanup(&st);
    CloseWindow(win);
}

static void test_backscreen(void)
{
    struct SurfaceTrack st;
    struct Screen *scr;
    struct Window *win;
    struct Rectangle inner, dest;
    struct Region *shape;

    printf("== window on a second (back) screen ==\n");

    scr = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE, SA_Behind, TRUE,
        SA_Title, (IPTR)"surfacetrack back screen", SA_ShowTitle, TRUE, TAG_DONE);
    if (!CHECK(scr != NULL))
        return;

    win = OpenWindowTags(NULL, WA_CustomScreen, (IPTR)scr,
        WA_Left, 50, WA_Top, 50, WA_Width, 240, WA_Height, 160,
        WA_Title, (IPTR)"back", WA_DragBar, TRUE, TAG_DONE);
    if (!CHECK(win != NULL))
    {
        CloseScreen(scr);
        return;
    }
    if (!CHECK(st_Init(&st, NULL)))
    {
        CloseWindow(win);
        CloseScreen(scr);
        return;
    }

    CHECK(st_WindowInnerRect(win, 0, 0, 0, 0, &inner));
    compare("back screen initial", &st, win, &inner, &dest, &shape);
    CHECK(st_Changed(&st));
    CHECK(dest.MinX == win->LeftEdge + win->BorderLeft);
    CHECK(dest.MinY == win->TopEdge + win->BorderTop);
    /* alone on its screen: everything visible (screen depth does not matter to layers) */
    CHECK(shape == NULL);

    MoveWindow(win, 20, 10);
    settle();
    compare("back screen moved", &st, win, &inner, &dest, &shape);
    CHECK(st_Changed(&st));

    ScreenToFront(scr);
    settle();
    compare("screen to front", &st, win, &inner, &dest, &shape);
    CHECK(!st_Changed(&st));
    ScreenToBack(scr);
    settle();

    st_Cleanup(&st);
    CloseWindow(win);
    CloseScreen(scr);
}

/*****************************************************************************/

static struct Window *notified;

static void close_cb(struct Window *window, APTR userData)
{
    if (userData == (APTR)0x1234)
        notified = window;
}

static void test_notify(struct Screen *scr)
{
    struct SurfaceTrackNotify n;
    struct Window *win;

    printf("== SNOTIFY_BEFORE_CLOSEWINDOW safety net ==\n");

    memset(&n, 0, sizeof(n));
    if (!CHECK(st_StartCloseNotify(&n, close_cb, (APTR)0x1234)))
        return;

    win = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr,
        WA_Left, 200, WA_Top, 200, WA_Width, 100, WA_Height, 60,
        WA_Title, (IPTR)"notify", TAG_DONE);
    CHECK(win != NULL);
    notified = NULL;
    if (win)
        CloseWindow(win);
    CHECK(notified == win);
    printf("  callback saw window %p (closed %p)\n", notified, win);

    st_EndCloseNotify(&n);
}

int main(void)
{
    struct Screen *scr;

    scr = LockPubScreen(NULL);
    if (!scr)
    {
        printf("FAIL: cannot lock the public screen\n");
        return RETURN_FAIL;
    }

    test_window(scr);
    test_gzz(scr);
    test_backscreen();
    test_notify(scr);

    UnlockPubScreen(NULL, scr);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}
