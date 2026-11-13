/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test for cgxvideo.library.

    Without the CGXVideoNullBackend variable it exercises the real
    display backend: a YUYV layer on the public screen attached to a
    window, filled with colour bars, moved, resized and covered by a
    second window. With the variable set it runs the handle state
    machine tests against the null backend (which keeps the source in
    RAM and displays nothing).

    Exit code 0 when every call succeeded, 5 (WARN) when the screen has
    no video overlay, 20 on a failure.
*/

#include <dos/dos.h>
#include <dos/var.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <proto/cgxvideo.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <string.h>

struct Library *CGXVideoBase;

static int failures;

#define CHECK(cond) check((cond), #cond, __LINE__)

static void check(BOOL ok, const char *what, int line)
{
    if (!ok)
    {
        printf("FAIL line %d: %s\n", line, what);
        failures++;
    }
}

static struct VLayerHandle *create(struct Screen *scr, ULONG type,
    ULONG w, ULONG h, ULONG modulo, ULONG *err)
{
    struct TagItem tags[] =
    {
        { VOA_SrcType,   type   },
        { VOA_SrcWidth,  w      },
        { VOA_SrcHeight, h      },
        { VOA_Error,     (IPTR)err },
        { modulo ? VOA_Modulo : TAG_IGNORE, modulo },
        { TAG_DONE,      0      }
    };

    *err = ~0;
    return CreateVLayerHandleTagList(scr, tags);
}

/*****************************************************************************
    Null backend: state machine
*****************************************************************************/

static void test_create(struct Screen *scr)
{
    struct VLayerHandle *vh;
    UBYTE *buf;
    ULONG err;

    /* planar: even sizes only, modulo is the Y pitch (= width) */
    CHECK(create(scr, SRCFMT_YCbCr420, 63, 32, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(scr, SRCFMT_YCbCr420, 64, 31, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    vh = create(scr, SRCFMT_YCbCr420, 64, 32, 0, &err);
    CHECK(vh != NULL);
    CHECK(err == VOERR_OK);
    if (vh)
    {
        CHECK(GetVLayerAttr(vh, VOA_Modulo) == 64);
        CHECK(LockVLayer(vh) == TRUE);
        buf = (UBYTE *)GetVLayerAttr(vh, VOA_BaseAddress);
        CHECK(buf != NULL);
        CHECK(GetVLayerAttr(vh, VOA_BaseOffset0) == 0);
        CHECK(GetVLayerAttr(vh, VOA_BaseOffset1) == 64 * 32);
        CHECK(GetVLayerAttr(vh, VOA_BaseOffset2) == 64 * 32 + 32 * 16);
        if (buf)
            memset(buf, 0x80, 64 * 32 * 3 / 2);
        UnlockVLayer(vh);
        DeleteVLayerHandle(vh);
    }
    /* semi-planar (AROS extension): UV plane after Y, no third plane */
    CHECK(create(scr, SRCFMT_YCbCr420_NV12, 64, 31, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    vh = create(scr, SRCFMT_YCbCr420_NV12, 64, 32, 0, &err);
    CHECK(vh != NULL);
    if (vh)
    {
        CHECK(GetVLayerAttr(vh, VOA_Modulo) == 64);
        CHECK(LockVLayer(vh) == TRUE);
        CHECK(GetVLayerAttr(vh, VOA_BaseOffset1) == 64 * 32);
        CHECK(GetVLayerAttr(vh, VOA_BaseOffset2) == 0);
        UnlockVLayer(vh);
        DeleteVLayerHandle(vh);
    }
    CHECK(QueryVLayerAttr(scr, VSQ_SupportedFormats) & VSQ_FMT_NV12);
    CHECK(create(scr, 99, 64, 32, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(scr, SRCFMT_RGB16PC, 0, 32, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(scr, SRCFMT_RGB16PC, 64, 0, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(scr, SRCFMT_RGB16PC, 64, 32, 64, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(NULL, SRCFMT_RGB16PC, 64, 32, 0, &err) == NULL);
    CHECK(err == VOERR_INVSCRMODE);

    vh = create(scr, SRCFMT_YCbCr16, 64, 32, 256, &err);
    CHECK(vh != NULL);
    CHECK(err == VOERR_OK);
    if (vh)
    {
        CHECK(GetVLayerAttr(vh, VOA_Modulo) == 256);
        DeleteVLayerHandle(vh);
    }

    CHECK(QueryVLayerAttr(scr, VSQ_SupportedFeatures) & VSQ_FEAT_OVERLAY);
    CHECK(QueryVLayerAttr(scr, VSQ_SupportedFormats) & VSQ_FMT_YUV420_PLANAR);
}

static void test_states(struct Screen *scr, struct Window *win)
{
    struct VLayerHandle *vh;
    UBYTE *buf;
    ULONG err;

    vh = create(scr, SRCFMT_RGB16PC, 64, 32, 0, &err);
    CHECK(vh != NULL);
    if (!vh)
        return;

    CHECK(GetVLayerAttr(vh, VOA_Width) == 64);
    CHECK(GetVLayerAttr(vh, VOA_Height) == 32);
    CHECK(GetVLayerAttr(vh, VOA_Modulo) == 128);
    CHECK(GetVLayerAttr(vh, VOA_ColorKey) == (IPTR)-1);
    CHECK(GetVLayerAttr(vh, VOA_ColorKeyPen) == (IPTR)-1);

    /* Buffer is only reachable while locked */
    CHECK(GetVLayerAttr(vh, VOA_BaseAddress) == 0);
    CHECK(LockVLayer(vh) == TRUE);
    CHECK(LockVLayer(vh) == FALSE);
    buf = (UBYTE *)GetVLayerAttr(vh, VOA_BaseAddress);
    CHECK(buf != NULL);
    if (buf)
        memset(buf, 0x5a, 128 * 32);

    CHECK(AttachVLayerTagList(vh, win, NULL) != 0);
    CHECK(DetachVLayer(vh) != 0);
    UnlockVLayer(vh);
    CHECK(GetVLayerAttr(vh, VOA_BaseAddress) == 0);

    CHECK(AttachVLayerTagList(vh, NULL, NULL) != 0);
    CHECK(AttachVLayerTagList(vh, win, NULL) == 0);
    CHECK(AttachVLayerTagList(vh, win, NULL) != 0);

    /* Locking is allowed while attached, detaching is not */
    CHECK(LockVLayer(vh) == TRUE);
    CHECK(DetachVLayer(vh) != 0);
    UnlockVLayer(vh);
    CHECK(DetachVLayer(vh) == 0);
    CHECK(DetachVLayer(vh) != 0);

    /* Deletion must be safe while attached and locked */
    CHECK(AttachVLayerTagList(vh, win, NULL) == 0);
    CHECK(LockVLayer(vh) == TRUE);
    DeleteVLayerHandle(vh);

    DeleteVLayerHandle(NULL);
    SwapVLayerBuffer(NULL);
}

static int run_null_mode(struct Screen *scr, struct Window *win)
{
    printf("null backend mode (CGXVideoNullBackend is set)\n");
    test_create(scr);
    test_states(scr, win);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}

/*****************************************************************************
    Real backend
*****************************************************************************/

/* 8 colour bars in YUYV (Y0 U Y1 V per pixel pair), shifted by phase */
static void fill_bars(UBYTE *base, ULONG width, ULONG height, ULONG pitch, ULONG phase)
{
    static const UBYTE bars[8][3] =
    {   /*  Y     U     V  */
        { 0xeb, 0x80, 0x80 },   /* white   */
        { 0xd2, 0x10, 0x92 },   /* yellow  */
        { 0xaa, 0xa6, 0x10 },   /* cyan    */
        { 0x91, 0x36, 0x22 },   /* green   */
        { 0x6a, 0xca, 0xde },   /* magenta */
        { 0x51, 0x5a, 0xf0 },   /* red     */
        { 0x29, 0xf0, 0x6e },   /* blue    */
        { 0x10, 0x80, 0x80 }    /* black   */
    };
    ULONG x, y;

    for (y = 0; y < height; y++)
    {
        UBYTE *row = base + y * pitch;

        for (x = 0; x < width; x += 2)
        {
            const UBYTE *c = bars[(((x + phase) * 8) / width) & 7];

            row[x * 2 + 0] = c[0];
            row[x * 2 + 1] = c[1];
            row[x * 2 + 2] = c[0];
            row[x * 2 + 3] = c[2];
        }
    }
}

/* The same bars as semi-planar NV12: Y plane (pitch), interleaved UV plane (pitch) at the reported offset */
static void fill_bars_nv12(UBYTE *base, ULONG width, ULONG height, ULONG pitch, ULONG uvoff, ULONG phase)
{
    static const UBYTE bars[8][3] =
    {
        { 0xeb, 0x80, 0x80 }, { 0xd2, 0x10, 0x92 }, { 0xaa, 0xa6, 0x10 }, { 0x91, 0x36, 0x22 },
        { 0x6a, 0xca, 0xde }, { 0x51, 0x5a, 0xf0 }, { 0x29, 0xf0, 0x6e }, { 0x10, 0x80, 0x80 }
    };
    UBYTE *uvplane = base + uvoff;
    ULONG x, y;

    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            base[y * pitch + x] = bars[(((x + phase) * 8) / width) & 7][0];
    for (y = 0; y < height / 2; y++)
    {
        for (x = 0; x < width / 2; x++)
        {
            const UBYTE *c = bars[(((x * 2 + phase) * 8) / width) & 7];

            uvplane[y * pitch + x * 2]     = c[1];
            uvplane[y * pitch + x * 2 + 1] = c[2];
        }
    }
}

/* The same bars as planar I420, written into the three planes: Y (pitch), U and V (pitch/2) at the reported offsets */
static void fill_bars_i420(UBYTE *base, ULONG width, ULONG height, ULONG pitch,
                           ULONG uoff, ULONG voff, ULONG phase)
{
    static const UBYTE bars[8][3] =
    {
        { 0xeb, 0x80, 0x80 }, { 0xd2, 0x10, 0x92 }, { 0xaa, 0xa6, 0x10 }, { 0x91, 0x36, 0x22 },
        { 0x6a, 0xca, 0xde }, { 0x51, 0x5a, 0xf0 }, { 0x29, 0xf0, 0x6e }, { 0x10, 0x80, 0x80 }
    };
    UBYTE *uplane = base + uoff;
    UBYTE *vplane = base + voff;
    ULONG cpitch = pitch / 2;
    ULONG x, y;

    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            base[y * pitch + x] = bars[(((x + phase) * 8) / width) & 7][0];
    for (y = 0; y < height / 2; y++)
    {
        for (x = 0; x < width / 2; x++)
        {
            const UBYTE *c = bars[(((x * 2 + phase) * 8) / width) & 7];

            uplane[y * cpitch + x] = c[1];
            vplane[y * cpitch + x] = c[2];
        }
    }
}

static ULONG srctype = SRCFMT_YCbCr16;

static BOOL draw_frame(struct VLayerHandle *vh, ULONG w, ULONG h, ULONG phase, BOOL verbose)
{
    UBYTE *base;
    ULONG pitch, uoff = 0, voff = 0;
    BOOL planar = (srctype == SRCFMT_YCbCr420);
    BOOL nv12   = (srctype == SRCFMT_YCbCr420_NV12);

    if (!LockVLayer(vh))
    {
        printf("  LockVLayer failed\n");
        return FALSE;
    }
    base  = (UBYTE *)GetVLayerAttr(vh, VOA_BaseAddress);
    pitch = GetVLayerAttr(vh, VOA_Modulo);
    uoff  = GetVLayerAttr(vh, VOA_BaseOffset1);
    voff  = GetVLayerAttr(vh, VOA_BaseOffset2);
    if (verbose)
    {
        printf("  locked: base %p pitch %lu", base, (unsigned long)pitch);
        if (planar)
            printf(", U at +%lu, V at +%lu", (unsigned long)uoff, (unsigned long)voff);
        if (nv12)
            printf(", UV at +%lu", (unsigned long)uoff);
        printf("\n");
    }
    if (!base || pitch < ((planar || nv12) ? w : w * 2) ||
        (planar && (!uoff || !voff || voff <= uoff)) || (nv12 && (!uoff || voff)))
    {
        printf("  bad lock result\n");
        UnlockVLayer(vh);
        return FALSE;
    }
    if (planar)
        fill_bars_i420(base, w, h, pitch, uoff, voff, phase);
    else if (nv12)
        fill_bars_nv12(base, w, h, pitch, uoff, phase);
    else
        fill_bars(base, w, h, pitch, phase);
    UnlockVLayer(vh);
    return TRUE;
}

static int run_real_mode(struct Screen *scr, struct Window *win)
{
    const ULONG w = 320, h = 240;
    struct VLayerHandle *vh;
    struct Window *win2;
    struct TagItem tags[] =
    {
        { VOA_SrcType,      SRCFMT_YCbCr16 },
        { VOA_SrcWidth,     w              },
        { VOA_SrcHeight,    h              },
        { VOA_Error,        0              },
        { VOA_DoubleBuffer, FALSE          },
        { TAG_DONE,         0              }
    };
    ULONG err = ~0, features, formats, maxw, step;
    BOOL dbl;

    printf("real backend mode\n");

    features = QueryVLayerAttr(scr, VSQ_SupportedFeatures);
    formats  = QueryVLayerAttr(scr, VSQ_SupportedFormats);
    maxw     = QueryVLayerAttr(scr, VSQ_MaxWidth);
    printf("QueryVLayerAttr: features 0x%lx formats 0x%lx maxwidth %lu\n",
           (unsigned long)features, (unsigned long)formats, (unsigned long)maxw);

    if (!(features & VSQ_FEAT_OVERLAY))
    {
        printf("no video overlay on this screen\n");
        CHECK(create(scr, SRCFMT_YCbCr16, w, h, 0, &err) == NULL);
        CHECK(err == VOERR_INVSCRMODE);
        printf("CreateVLayerHandleTagList: error %lu (VOERR_INVSCRMODE expected)\n", (unsigned long)err);
        printf("%s (no overlay): %d failure(s)\n", failures ? "FAIL" : "WARN", failures);
        return failures ? RETURN_FAIL : RETURN_WARN;
    }

    dbl = (features & VSQ_FEAT_DOUBLEBUFFER) != 0;
    /* planar I420 when offered, else semi-planar NV12 (the GB207 windows), else packed */
    if (formats & VSQ_FMT_YUV420_PLANAR)
        srctype = SRCFMT_YCbCr420;
    else if (formats & VSQ_FMT_NV12)
        srctype = SRCFMT_YCbCr420_NV12;
    tags[0].ti_Data = srctype;
    tags[3].ti_Data = (IPTR)&err;
    tags[4].ti_Data = dbl;

    vh = CreateVLayerHandleTagList(scr, tags);
    printf("CreateVLayerHandleTagList(%s %lux%lu%s): %p error %lu\n",
           srctype == SRCFMT_YCbCr420 ? "YCbCr420 planar I420" :
           srctype == SRCFMT_YCbCr420_NV12 ? "YCbCr420_NV12 semi-planar" : "YCbCr16 YUYV",
           (unsigned long)w, (unsigned long)h, dbl ? ", double buffered" : "", vh, (unsigned long)err);
    CHECK(vh != NULL);
    CHECK(err == VOERR_OK);
    if (!vh)
        return RETURN_FAIL;

    CHECK(GetVLayerAttr(vh, VOA_Width) == w);
    CHECK(GetVLayerAttr(vh, VOA_Height) == h);
    CHECK(GetVLayerAttr(vh, VOA_BaseAddress) == 0);
    printf("VOA_Modulo (unlocked): %lu\n", (unsigned long)GetVLayerAttr(vh, VOA_Modulo));

    err = AttachVLayerTagList(vh, win, NULL);
    printf("AttachVLayerTagList: %lu\n", (unsigned long)err);
    CHECK(err == 0);
    if (err)
    {
        DeleteVLayerHandle(vh);
        return RETURN_FAIL;
    }

    printf("LockVLayer / fill colour bars / UnlockVLayer\n");
    CHECK(draw_frame(vh, w, h, 0, TRUE));
    if (dbl)
    {
        printf("SwapVLayerBuffer\n");
        SwapVLayerBuffer(vh);
    }

    win2 = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr,
        WA_Left, win->LeftEdge + 40, WA_Top, win->TopEdge + 40,
        WA_Width, 200, WA_Height, 150, WA_Title, (IPTR)"vlayertest cover",
        WA_DragBar, TRUE, WA_DepthGadget, TRUE, TAG_DONE);
    printf("cover window: %p\n", win2);

    printf("moving/resizing for 3 seconds (%s)\n", dbl ? "swapping each frame" : "single buffer");
    for (step = 0; step < 60; step++)
    {
        LONG dx = (step & 1) ? 6 : -6;
        LONG dy = (step & 2) ? 4 : -4;

        MoveWindow(win, dx, dy);
        if ((step % 10) == 5)
            SizeWindow(win, 20, 15);
        else if ((step % 10) == 0 && step)
            SizeWindow(win, -20, -15);
        if (win2)
            MoveWindow(win2, (step & 1) ? -10 : 10, (step & 4) ? 3 : -3);

        if (!draw_frame(vh, w, h, step * 4, FALSE))
        {
            failures++;
            break;
        }
        if (dbl)
            SwapVLayerBuffer(vh);
        Delay(2);
    }

    if (win2)
        CloseWindow(win2);

    err = DetachVLayer(vh);
    printf("DetachVLayer: %lu\n", (unsigned long)err);
    CHECK(err == 0);

    printf("DeleteVLayerHandle\n");
    DeleteVLayerHandle(vh);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}

/*****************************************************************************/

int main(void)
{
    struct Screen *scr;
    struct Window *win;
    TEXT buf[2];
    int rc;

    CGXVideoBase = OpenLibrary("cgxvideo.library", 41);
    if (!CGXVideoBase)
    {
        printf("FAIL: cannot open cgxvideo.library\n");
        return RETURN_FAIL;
    }

    scr = LockPubScreen(NULL);
    win = scr ? OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr,
        WA_Left, 60, WA_Top, 60,
        WA_InnerWidth, 320, WA_InnerHeight, 240, WA_Title, (IPTR)"vlayertest",   /* = the real-mode source: unscaled at first */
        WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_SizeGadget, TRUE,
        WA_MinWidth, 100, WA_MinHeight, 80, WA_MaxWidth, -1, WA_MaxHeight, -1,
        TAG_DONE) : NULL;
    if (!win)
    {
        printf("FAIL: cannot open a window\n");
        if (scr)
            UnlockPubScreen(NULL, scr);
        CloseLibrary(CGXVideoBase);
        return RETURN_FAIL;
    }

    if (GetVar("CGXVideoNullBackend", buf, sizeof(buf), 0) >= 0)
        rc = run_null_mode(scr, win);
    else
        rc = run_real_mode(scr, win);

    CloseWindow(win);
    UnlockPubScreen(NULL, scr);
    CloseLibrary(CGXVideoBase);

    return rc;
}
