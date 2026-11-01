/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native regression for palette screens on a truecolor display.
    It opens real Intuition screens and reads the compositor's actual target.
    Palette setters are followed only by timer waits and read-only reads;
    no redraw, LoadView, RethinkDisplay, or getter-triggered flush is used.
*/

#define __OOP_NOATTRBASES__

#include <exec/memory.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <graphics/displayinfo.h>
#include <graphics/modeid.h>
#include <intuition/screens.h>
#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/oop.h>

#include <stdio.h>
#include <stddef.h>
#include <string.h>

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddDisplayAttrBase;
static OOP_AttrBase HiddCompositorAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddColorMapAttrBase;

#define MAX_WAIT_FRAMES 12
#define SAMPLE_X 32
#define SAMPLE_Y 32
#define FAIL_REDRAWREGION "compositor.fail.redrawregion"

struct test_state
{
    struct MsgPort *port;
    struct timerequest *timer;
    BOOL timer_open;
    OOP_Object *display;
    OOP_Object *compositor;
    ULONG checks;
    ULONG failures;
    ULONG skips;
};

static void check(struct test_state *t, BOOL condition, const char *name)
{
    t->checks++;
    if (!condition)
    {
        t->failures++;
        printf("FAIL: %s\n", name);
    }
    else
        printf("PASS: %s\n", name);
}

static void skip(struct test_state *t, const char *name)
{
    t->skips++;
    printf("SKIP: %s\n", name);
}

static BOOL next_frame(struct test_state *t)
{
    t->timer->tr_node.io_Command = TR_ADDREQUEST;
    t->timer->tr_time.tv_secs = 0;
    t->timer->tr_time.tv_micro = 1;
    return DoIO((struct IORequest *)t->timer) == 0;
}

static OOP_Object *composed(struct test_state *t)
{
    OOP_Object *target = NULL;
    OOP_GetAttr(t->compositor, aHidd_Compositor_ComposedBitMap, (IPTR *)&target);
    return target;
}

static BOOL is_rgb(OOP_Object *bm, LONG x, LONG y, ULONG rgb, ULONG *actual)
{
    UBYTE argb[4] = { 0, 0, 0, 0 };
    IPTR width = 0, height = 0;

    if (!bm)
        return FALSE;
    OOP_GetAttr(bm, aHidd_BitMap_Width, &width);
    OOP_GetAttr(bm, aHidd_BitMap_Height, &height);
    if (x < 0 || y < 0 || x >= width || y >= height)
        return FALSE;
    HIDD_BM_GetImage(bm, argb, sizeof(argb), x, y, 1, 1, vHidd_StdPixFmt_ARGB32);
    *actual = ((ULONG)argb[1] << 16) | ((ULONG)argb[2] << 8) | argb[3];
    return *actual == rgb;
}

/* Read-only polling: composition must happen independently in the worker. */
static void expect_rgb(struct test_state *t, LONG x, LONG y, ULONG rgb, const char *name)
{
    ULONG frame, actual = 0;
    BOOL found = FALSE;

    for (frame = 0; frame <= MAX_WAIT_FRAMES; frame++)
    {
        if (is_rgb(composed(t), x, y, rgb, &actual))
        {
            found = TRUE;
            break;
        }
        if (frame != MAX_WAIT_FRAMES && !next_frame(t))
            break;
    }
    check(t, found, name);
    printf("  output (%ld,%ld): RGB %06lx, expected %06lx, waited %lu VBlanks\n",
           (long)x, (long)y, (unsigned long)actual, (unsigned long)rgb,
           (unsigned long)frame);
}

static void expect_pen(struct test_state *t, OOP_Object *bm, LONG x, LONG y,
                       ULONG pen, const char *name)
{
    ULONG pixel = ~(ULONG)0;
    HIDD_BM_GetImage(bm, (UBYTE *)&pixel, sizeof(pixel), x, y, 1, 1,
                     vHidd_StdPixFmt_Native32);
    check(t, pixel == pen, name);
}

static struct Screen *open_palette_screen(ULONG mode, ULONG width, ULONG height)
{
    return OpenScreenTags(NULL,
                          SA_DisplayID, mode,
                          SA_Width, width,
                          SA_Height, height,
                          SA_Depth, 8,
                          SA_Type, CUSTOMSCREEN,
                          SA_Quiet, TRUE,
                          SA_ShowTitle, FALSE,
                          SA_Draggable, TRUE,
                          SA_CompositingFlags, COMPF_ABOVE,
                          TAG_DONE);
}

/* Choose an exact OS database entry, independently of BestModeID preferences. */
static ULONG palette_mode(ULONG public_mode, ULONG width, ULONG height)
{
    ULONG id = INVALID_ID;

    while ((id = NextDisplayInfo(id)) != INVALID_ID)
    {
        struct DimensionInfo dim = {0};
        ULONG bytes;

        if ((id & MONITOR_ID_MASK) != (public_mode & MONITOR_ID_MASK))
            continue;
        bytes = GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, id);
        if (bytes >= offsetof(struct DimensionInfo, Nominal) + sizeof(dim.Nominal) &&
            dim.MaxDepth == 8 && dim.Nominal.MaxX - dim.Nominal.MinX + 1 == width &&
            dim.Nominal.MaxY - dim.Nominal.MinY + 1 == height)
            return id;
    }
    return INVALID_ID;
}

static void fill_pen(struct Screen *screen, ULONG pen)
{
    SetAPen(&screen->RastPort, pen);
    RectFill(&screen->RastPort, 0, 0, screen->Width - 1, screen->Height - 1);
}

static void load_one(struct Screen *screen, ULONG pen, ULONG r, ULONG g, ULONG b)
{
    ULONG table[] = { (1UL << 16) | pen, r, g, b, 0 };
    LoadRGB32(&screen->ViewPort, table);
}

/* Detach before disposing: the asynchronous reader must observe a live map. */
static void growth_test(struct test_state *t, struct Screen *screen)
{
    OOP_Object *bm = HIDD_BM_OBJ(screen->RastPort.BitMap);
    OOP_Object *small, *old;
    IPTR entries = 0;
    ULONG i;
    struct TagItem tags[] =
    {
        { aHidd_ColorMap_NumEntries, 16 },
        { TAG_DONE, 0 }
    };

    small = OOP_NewObject(NULL, CLID_Hidd_ColorMap, tags);
    check(t, small != NULL, "allocate 16-entry map for live growth");
    if (!small)
        return;
    old = HIDD_BM_SetColorMap(bm, small);
    OOP_GetAttr(small, aHidd_ColorMap_NumEntries, &entries);
    check(t, entries == 16, "replacement map starts with 16 entries");

    /* Queue a palette frame, then grow the same map while it may be sampled. */
    SetRGB32(&screen->ViewPort, 1, 0xffffffff, 0, 0);
    SetRGB32(&screen->ViewPort, 200, 0, 0xffffffff, 0);
    OOP_GetAttr(small, aHidd_ColorMap_NumEntries, &entries);
    check(t, entries >= 201, "SetRGB32 grows the active map beyond entry 16");
    fill_pen(screen, 200);
    expect_rgb(t, SAMPLE_X, SAMPLE_Y, 0x00ff00,
               "grown map renders pen 200 through the compositor");
    expect_pen(t, bm, SAMPLE_X, SAMPLE_Y, 200, "growth preserves source pen 200");

    /* A burst of per-pen changes must converge without another draw call. */
    for (i = 0; i < 256; i++)
        SetRGB32(&screen->ViewPort, i, 0, 0, 0xffffffff);
    expect_rgb(t, SAMPLE_X, SAMPLE_Y, 0x0000ff,
               "256 SetRGB32 calls converge through the palette worker");
    expect_pen(t, bm, SAMPLE_X, SAMPLE_Y, 200, "per-pen burst preserves source indices");
    load_one(screen, 200, 0xffffffff, 0xffffffff, 0);
    expect_rgb(t, SAMPLE_X, SAMPLE_Y, 0xffff00,
               "LoadRGB32 updates a grown map without a draw call");
    expect_pen(t, bm, SAMPLE_X, SAMPLE_Y, 200, "LoadRGB32 preserves source indices");

    check(t, HIDD_BM_SetColorMap(bm, old) == small, "restore original map before disposal");
    OOP_DisposeObject(small);
}

/* Optional fault-build case: never replace or remove another test's marker. */
static void nomem_test(struct test_state *t, struct Screen *screen)
{
    struct SignalSemaphore *fail;
    ULONG i, actual = 0;
    BOOL installed = FALSE, unchanged = TRUE, timer_ok = TRUE;

    if (!is_rgb(composed(t), SAMPLE_X, SAMPLE_Y, 0x00ff00, &actual))
    {
        check(t, FALSE, "NOMEM starts with the verified green composed pixel");
        return;
    }
    fail = AllocMem(sizeof(*fail), MEMF_PUBLIC | MEMF_CLEAR);
    check(t, fail != NULL, "NOMEM allocates its own fault marker");
    if (!fail)
        return;
    fail->ss_Link.ln_Name = (STRPTR)FAIL_REDRAWREGION;
    fail->ss_Link.ln_Pri = -128;
    /*
     * Forbid makes this check-and-add indivisible on UP AROS. On SMP it is
     * CPU-local, so also verify pointer identity after AddSemaphore. The
     * test requires an exclusive invocation; it claims no SMP admission lock.
     */
    Forbid();
    if (!FindSemaphore(FAIL_REDRAWREGION))
    {
        AddSemaphore(fail);
        if (FindSemaphore(FAIL_REDRAWREGION) == fail)
            installed = TRUE;
        else
            RemSemaphore(fail);
    }
    Permit();
    check(t, installed, "NOMEM reserves an unused fault marker");
    if (!installed)
    {
        FreeMem(fail, sizeof(*fail));
        return;
    }

    /* Only the palette changes: no rendering operation may hide a lost retry. */
    SetRGB32(&screen->ViewPort, 1, 0, 0, 0xffffffff);
    for (i = 0; i < MAX_WAIT_FRAMES; i++)
    {
        if (!next_frame(t))
        {
            timer_ok = FALSE;
            break;
        }
        if (!is_rgb(composed(t), SAMPLE_X, SAMPLE_Y, 0x00ff00, &actual))
            unchanged = FALSE;
    }
    check(t, timer_ok, "NOMEM timer waits complete while redraw allocation fails");
    check(t, unchanged, "NOMEM preserves existing target pixels while allocation fails");
    printf("  NOMEM held for %lu VBlanks; final RGB %06lx (expected green)\n",
           (unsigned long)i, (unsigned long)actual);

    /* Remove only our node, even if an uncooperative test added another. */
    RemSemaphore(fail);
    FreeMem(fail, sizeof(*fail));
    expect_rgb(t, SAMPLE_X, SAMPLE_Y, 0x0000ff,
               "NOMEM pending palette update retries after the fault is removed");
    expect_pen(t, HIDD_BM_OBJ(screen->RastPort.BitMap), SAMPLE_X, SAMPLE_Y, 1,
               "NOMEM retry preserves source pen indices");
}

static BOOL obtain_bases(void)
{
    HiddBitMapAttrBase = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    HiddDisplayAttrBase = OOP_ObtainAttrBase(IID_Hidd_Display);
    HiddCompositorAttrBase = OOP_ObtainAttrBase(IID_Hidd_Compositor);
    HiddPixFmtAttrBase = OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    HiddSyncAttrBase = OOP_ObtainAttrBase(IID_Hidd_Sync);
    HiddColorMapAttrBase = OOP_ObtainAttrBase(IID_Hidd_ColorMap);
    return HiddBitMapAttrBase && HiddDisplayAttrBase && HiddCompositorAttrBase &&
           HiddPixFmtAttrBase && HiddSyncAttrBase && HiddColorMapAttrBase;
}

static void release_bases(void)
{
    if (HiddColorMapAttrBase) OOP_ReleaseAttrBase(IID_Hidd_ColorMap);
    if (HiddSyncAttrBase) OOP_ReleaseAttrBase(IID_Hidd_Sync);
    if (HiddPixFmtAttrBase) OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    if (HiddCompositorAttrBase) OOP_ReleaseAttrBase(IID_Hidd_Compositor);
    if (HiddDisplayAttrBase) OOP_ReleaseAttrBase(IID_Hidd_Display);
    if (HiddBitMapAttrBase) OOP_ReleaseAttrBase(IID_Hidd_BitMap);
}

int main(void)
{
    struct test_state t = { 0 };
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    BOOL nomem_ran = FALSE;
    struct Screen *pub = NULL, *back = NULL, *front = NULL;
    OOP_Object *dmenum = NULL, *sync, *pf, *bm;
    struct DimensionInfo dim;
    HIDDT_ModeID hmode = vHidd_ModeID_Invalid;
    ULONG mode, public_mode, width, height, palette_modes = 0, truecolor_modes = 0, dimbytes;
    IPTR model = 0, depth = 0, displayable = 0, capabilities = 0;
    IPTR displayid = 0;
    BOOL software = FALSE;
    ULONG i;

    rda = ReadArgs("NOMEM/S", args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "gfx_palette");
        return RETURN_FAIL;
    }
    printf("gfx_palette: real screens, independent VBlank refresh, native output reads\n");
    if (!obtain_bases())
    {
        printf("FAIL: required graphics HIDD interfaces unavailable\n");
        release_bases();
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    t.port = CreateMsgPort();
    if (t.port)
        t.timer = (struct timerequest *)CreateIORequest(t.port, sizeof(*t.timer));
    if (!t.timer || OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)t.timer, 0))
    {
        check(&t, FALSE, "open UNIT_VBLANK timer");
        goto out;
    }
    t.timer_open = TRUE;
    pub = LockPubScreen(NULL);
    check(&t, pub != NULL, "lock public screen");
    if (!pub)
        goto out;
    if (!IS_HIDD_BM(pub->RastPort.BitMap))
    {
        skip(&t, "public screen is not managed by a graphics HIDD");
        goto out;
    }
    bm = HIDD_BM_OBJ(pub->RastPort.BitMap);
    OOP_GetAttr(bm, aHidd_BitMap_Display, (IPTR *)&t.display);
    if (t.display)
    {
        OOP_GetAttr(t.display, aHidd_Display_Compositor, (IPTR *)&t.compositor);
        OOP_GetAttr(t.display, aHidd_Display_DMEnumerator, (IPTR *)&dmenum);
    }
    check(&t, t.display && t.compositor && dmenum, "public display has compositor and mode enumerator");
    if (!t.display || !t.compositor || !dmenum)
        goto out;
    OOP_GetAttr(t.compositor, aHidd_Compositor_Capabilities, &capabilities);
    OOP_GetAttr(t.display, aHidd_Display_DisplayID, &displayid);
    public_mode = GetVPModeID(&pub->ViewPort);
    width = pub->Width;
    height = pub->Height;
    UnlockPubScreen(NULL, pub);
    pub = NULL;

    while ((hmode = HIDD_DMEnum_NextModeID(dmenum, hmode, &sync, &pf)) != vHidd_ModeID_Invalid)
    {
        IPTR stdpf = 0;
        OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, &model);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
        OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
        if (model == vHidd_ColorModel_TrueColor)
            truecolor_modes++;
        if (model == vHidd_ColorModel_Palette && depth == 8 && stdpf == vHidd_StdPixFmt_LUT8)
            palette_modes++;
        {
            struct DimensionInfo candidate = {0};
            struct DisplayInfo info = {0};
            ULONG id = displayid | hmode;
            ULONG bytes = GetDisplayInfoData(NULL, (UBYTE *)&candidate, sizeof(candidate), DTAG_DIMS, id);
            GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_DISP, id);
            printf("  candidate %08lx: HIDD depth %lu, OS depth %u, bytes %lu, flags %08lx, RGB %u/%u/%u\n",
                   (unsigned long)id, (unsigned long)depth, candidate.MaxDepth,
                   (unsigned long)bytes, (unsigned long)info.PropertyFlags,
                   info.RedBits, info.GreenBits, info.BlueBits);
        }
    }
    {
        HIDDT_ModeID *ids = HIDD_DMEnum_QueryModeIDs(dmenum, NULL);
        printf("  HIDD query:");
        if (ids)
        {
            HIDDT_ModeID *at;

            for (at = ids; *at != vHidd_ModeID_Invalid; at++)
                printf(" %08lx", (unsigned long)*at);
            HIDD_DMEnum_ReleaseModeIDs(dmenum, ids);
        }
        printf("\n");
    }
    printf("  display modes: %lu truecolor, %lu LUT8\n",
           (unsigned long)truecolor_modes, (unsigned long)palette_modes);
    if (!truecolor_modes && !palette_modes)
    {
        skip(&t, "display has neither truecolor nor LUT8 modes");
        goto out;
    }
    if (!palette_modes && !(capabilities & COMPF_ABOVE))
    {
        skip(&t, "display has no indexed mode and cannot compose software screens");
        goto out;
    }
    check(&t, palette_modes != 0, "LUT8 modes are enumerated by this display");
    if (!palette_modes)
        goto out;
    mode = palette_mode(public_mode, width, height);
    check(&t, mode != INVALID_ID, "graphics.library enumerates an indexed mode");
    if (mode == INVALID_ID)
        goto out;
    memset(&dim, 0, sizeof(dim));
    dimbytes = GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, mode);
    printf("  selected mode %08lx: depth %u, dimension bytes %lu\n",
           (unsigned long)mode, dim.MaxDepth, (unsigned long)dimbytes);
    check(&t, dimbytes >= offsetof(struct DimensionInfo, Nominal) + sizeof(dim.Nominal)
              && dim.MaxDepth == 8, "selected indexed mode has exactly eight-bit depth");
    if (dim.MaxDepth != 8)
        goto out;
    width = dim.Nominal.MaxX - dim.Nominal.MinX + 1;
    height = dim.Nominal.MaxY - dim.Nominal.MinY + 1;
    check(&t, width > 128 && height > 128, "mode is large enough for disjoint output samples");
    if (width <= 128 || height <= 128)
        goto out;

    back = open_palette_screen(mode, width, height);
    check(&t, back && IS_HIDD_BM(back->RastPort.BitMap), "open a full-size indexed Intuition screen");
    if (!back || !IS_HIDD_BM(back->RastPort.BitMap))
        goto out;
    bm = HIDD_BM_OBJ(back->RastPort.BitMap);
    OOP_GetAttr(bm, aHidd_BitMap_Displayable, &displayable);
    software = !displayable;
    printf("  indexed screen path: %s\n", software ? "generic software composition" : "native displayable");
    SetRGB32(&back->ViewPort, 1, 0xffffffff, 0, 0);
    fill_pen(back, 1);
    if (software)
    {
        check(&t, composed(&t) != NULL, "one covering software indexed screen still composes");
        expect_rgb(&t, SAMPLE_X, SAMPLE_Y, 0xff0000, "single screen renders its own palette");
        SetRGB32(&back->ViewPort, 1, 0, 0xffffffff, 0);
        expect_rgb(&t, SAMPLE_X, SAMPLE_Y, 0x00ff00, "SetRGB32 refreshes a composed non-mirrored screen");
        expect_pen(&t, bm, SAMPLE_X, SAMPLE_Y, 1, "single screen retains pen indices");
        if (args[0])
        {
            nomem_ran = TRUE;
            nomem_test(&t, back);
        }
        growth_test(&t, back);
    }
    else
    {
        check(&t, composed(&t) == NULL, "one covering native indexed screen remains passthrough");
        skip(&t, "software-only single-screen and growth cases need a truecolor-only driver");
    }

    OOP_GetAttr(t.compositor, aHidd_Compositor_Capabilities, &capabilities);
    if (!(capabilities & COMPF_ABOVE))
    {
        skip(&t, "display cannot compose dragged screen stacks");
        goto pending_close;
    }
    SetRGB32(&back->ViewPort, 1, 0, 0, 0xffffffff);
    fill_pen(back, 1);
    front = open_palette_screen(mode, width, height);
    check(&t, front && IS_HIDD_BM(front->RastPort.BitMap), "open a second indexed screen");
    if (!front || !IS_HIDD_BM(front->RastPort.BitMap))
        goto out;
    SetRGB32(&front->ViewPort, 1, 0xffffffff, 0, 0);
    fill_pen(front, 1);
    ScreenPosition(front, SPOS_ABSOLUTE, 0, height / 2, 0, 0);
    check(&t, front->TopEdge >= height / 4, "drag exposes the back screen");
    if (!software && !composed(&t))
    {
        skip(&t, "native indexed stack uses display planes rather than software composition");
        CloseScreen(front);
        front = NULL;
        goto pending_close;
    }
    check(&t, composed(&t) != NULL, "dragged indexed stack uses a composed target");
    expect_rgb(&t, SAMPLE_X, SAMPLE_Y, 0x0000ff, "back screen keeps its independent blue palette");
    expect_rgb(&t, SAMPLE_X, front->TopEdge + SAMPLE_Y, 0xff0000,
               "front screen keeps its independent red palette");
    SetRGB32(&back->ViewPort, 1, 0, 0xffffffff, 0);
    expect_rgb(&t, SAMPLE_X, SAMPLE_Y, 0x00ff00, "SetRGB32 refreshes an exposed background screen");
    expect_rgb(&t, SAMPLE_X, front->TopEdge + SAMPLE_Y, 0xff0000,
               "background palette changes do not recolor the front screen");
    load_one(front, 1, 0xffffffff, 0xffffffff, 0);
    expect_rgb(&t, SAMPLE_X, front->TopEdge + SAMPLE_Y, 0xffff00,
               "LoadRGB32 refreshes a composed front screen");
    expect_pen(&t, HIDD_BM_OBJ(front->RastPort.BitMap), SAMPLE_X, SAMPLE_Y, 1,
               "front composition preserves source indices");
    expect_pen(&t, bm, SAMPLE_X, SAMPLE_Y, 1, "background composition preserves source indices");
    CloseScreen(front);
    front = NULL;

pending_close:
    /* Queue work immediately before freeing its screen; no retained bitmap may be used. */
    for (i = 0; i < 8; i++)
        SetRGB32(&back->ViewPort, 1, i & 1 ? 0xffffffff : 0, 0, 0);
    if (CloseScreen(back))
    {
        check(&t, TRUE, "close a screen immediately after palette invalidation");
        back = NULL;
    }
    else
        check(&t, FALSE, "close a screen immediately after palette invalidation");
    for (i = 0; i < 3; i++)
        check(&t, next_frame(&t), "timer and display remain usable after pending close");

out:
    if (args[0] && !nomem_ran)
        check(&t, FALSE, "NOMEM requires an exercised generic software LUT8 screen");
    if (front) CloseScreen(front);
    if (back) CloseScreen(back);
    if (pub) UnlockPubScreen(NULL, pub);
    if (t.timer_open) CloseDevice((struct IORequest *)t.timer);
    if (t.timer) DeleteIORequest((struct IORequest *)t.timer);
    if (t.port) DeleteMsgPort(t.port);
    release_bases();
    FreeArgs(rda);
    printf("gfx_palette: %lu checks, %lu failures, %lu skipped cases\n",
           (unsigned long)t.checks, (unsigned long)t.failures, (unsigned long)t.skips);
    return t.failures ? RETURN_FAIL : 0;
}
