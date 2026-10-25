/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vlayerplay: animated colour bars in a cgxvideo.library video layer
    until the window is closed.

    SCREEN/K  - "OWN" opens a screen of its own (the second screen case),
                any other value is the name of a public screen to use;
                default: the default public screen
    DOUBLE/S  - double buffer (when the screen supports it)
    KEY/S     - colour key with backfill
    STATIC/S  - no movement
    RGB16/S   - SRCFMT_RGB16PC source instead of SRCFMT_YCbCr16
    YUV420/S  - SRCFMT_YCbCr420 (planar I420) source
    NV12/S    - SRCFMT_YCbCr420_NV12 (semi-planar, AROS extension) source
*/

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/cgxvideo.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <string.h>

struct Library *CGXVideoBase;

static const char version[] = "$VER: vlayerplay 1.0 (25.10.2026)";

#define TEMPLATE "SCREEN/K,DOUBLE/S,KEY/S,STATIC/S,RGB16/S,YUV420/S,NV12/S"
enum { ARG_SCREEN, ARG_DOUBLE, ARG_KEY, ARG_STATIC, ARG_RGB16, ARG_YUV420, ARG_NV12, ARG_COUNT };

#define SRC_W 320
#define SRC_H 240

static void fill_bars(UBYTE *base, ULONG width, ULONG height, ULONG pitch, ULONG frame)
{
    static const UBYTE bars[8][3] =
    {
        { 0xeb, 0x80, 0x80 }, { 0xd2, 0x10, 0x92 }, { 0xaa, 0xa6, 0x10 }, { 0x91, 0x36, 0x22 },
        { 0x6a, 0xca, 0xde }, { 0x51, 0x5a, 0xf0 }, { 0x29, 0xf0, 0x6e }, { 0x10, 0x80, 0x80 }
    };
    ULONG x, y, band = height / 8;

    for (y = 0; y < height; y++)
    {
        UBYTE *row = base + y * pitch;
        /* a moving dark line marks the frame rate */
        BOOL line = ((y + frame) % band) == 0;

        for (x = 0; x < width; x += 2)
        {
            const UBYTE *c = bars[(((x + frame * 2) * 8) / width) & 7];
            UBYTE yv = line ? 0x10 : c[0];

            row[x * 2 + 0] = yv;
            row[x * 2 + 1] = c[1];
            row[x * 2 + 2] = yv;
            row[x * 2 + 3] = c[2];
        }
    }
}

/*
 * The same 8 bars as planar I420 (SRCFMT_YCbCr420), written straight into
 * the three planes of the scan-out buffer: Y with the driver pitch
 * (VOA_Modulo), U and V at VOA_BaseOffset1/2 with pitch/2, height/2 rows.
 */
static void fill_bars_i420(UBYTE *base, ULONG width, ULONG height, ULONG pitch,
                           ULONG uoff, ULONG voff, ULONG frame)
{
    static const UBYTE bars[8][3] =
    {
        { 0xeb, 0x80, 0x80 }, { 0xd2, 0x10, 0x92 }, { 0xaa, 0xa6, 0x10 }, { 0x91, 0x36, 0x22 },
        { 0x6a, 0xca, 0xde }, { 0x51, 0x5a, 0xf0 }, { 0x29, 0xf0, 0x6e }, { 0x10, 0x80, 0x80 }
    };
    UBYTE *uplane = base + uoff;
    UBYTE *vplane = base + voff;
    ULONG cpitch = pitch / 2;
    ULONG x, y, band = height / 8;

    for (y = 0; y < height; y++)
    {
        UBYTE *row = base + y * pitch;
        BOOL line = ((y + frame) % band) == 0;

        for (x = 0; x < width; x++)
            row[x] = line ? 0x10 : bars[(((x + frame * 2) * 8) / width) & 7][0];
    }
    for (y = 0; y < height / 2; y++)
    {
        UBYTE *urow = uplane + y * cpitch;
        UBYTE *vrow = vplane + y * cpitch;

        for (x = 0; x < width / 2; x++)
        {
            const UBYTE *c = bars[(((x * 2 + frame * 2) * 8) / width) & 7];

            urow[x] = c[1];
            vrow[x] = c[2];
        }
    }
}

/*
 * The same 8 bars as semi-planar NV12 (SRCFMT_YCbCr420_NV12): Y plane with
 * the driver pitch, then one interleaved U,V plane (U byte, V byte per
 * pixel pair) at VOA_BaseOffset1 with the same pitch, height/2 rows.
 */
static void fill_bars_nv12(UBYTE *base, ULONG width, ULONG height, ULONG pitch,
                           ULONG uvoff, ULONG frame)
{
    static const UBYTE bars[8][3] =
    {
        { 0xeb, 0x80, 0x80 }, { 0xd2, 0x10, 0x92 }, { 0xaa, 0xa6, 0x10 }, { 0x91, 0x36, 0x22 },
        { 0x6a, 0xca, 0xde }, { 0x51, 0x5a, 0xf0 }, { 0x29, 0xf0, 0x6e }, { 0x10, 0x80, 0x80 }
    };
    UBYTE *uvplane = base + uvoff;
    ULONG x, y, band = height / 8;

    for (y = 0; y < height; y++)
    {
        UBYTE *row = base + y * pitch;
        BOOL line = ((y + frame) % band) == 0;

        for (x = 0; x < width; x++)
            row[x] = line ? 0x10 : bars[(((x + frame * 2) * 8) / width) & 7][0];
    }
    for (y = 0; y < height / 2; y++)
    {
        UBYTE *row = uvplane + y * pitch;

        for (x = 0; x < width / 2; x++)
        {
            const UBYTE *c = bars[(((x * 2 + frame * 2) * 8) / width) & 7];

            row[x * 2]     = c[1];
            row[x * 2 + 1] = c[2];
        }
    }
}

/* The same 8 bars as RGB565 little endian (SRCFMT_RGB16PC): a scan-out check without the YUV path */
static void fill_bars_rgb16(UBYTE *base, ULONG width, ULONG height, ULONG pitch, ULONG frame)
{
    static const UWORD bars[8] =
    {
        0xffff, 0xffe0, 0x07ff, 0x07e0, 0xf81f, 0xf800, 0x001f, 0x0000
    };
    ULONG x, y, band = height / 8;

    for (y = 0; y < height; y++)
    {
        UWORD *row = (UWORD *)(base + y * pitch);
        BOOL line = ((y + frame) % band) == 0;

        for (x = 0; x < width; x++)
            row[x] = line ? 0x0000 : bars[(((x + frame * 2) * 8) / width) & 7];
    }
}

static void print_query(struct Screen *scr)
{
    ULONG f = QueryVLayerAttr(scr, VSQ_SupportedFeatures);
    ULONG m = QueryVLayerAttr(scr, VSQ_SupportedFormats);

    printf("QueryVLayerAttr:\n");
    printf("  features 0x%lx:%s%s%s%s%s\n", (unsigned long)f,
           (f & VSQ_FEAT_OVERLAY)      ? " overlay"      : "",
           (f & VSQ_FEAT_DOUBLEBUFFER) ? " doublebuffer" : "",
           (f & VSQ_FEAT_MULTIBUFFER)  ? " multibuffer"  : "",
           (f & VSQ_FEAT_COLORKEYING)  ? " colorkeying"  : "",
           (f & VSQ_FEAT_FILTERING)    ? " filtering"    : "");
    printf("  formats 0x%lx:%s%s%s%s%s\n", (unsigned long)m,
           (m & VSQ_FMT_YUYV)          ? " YUYV"       : "",
           (m & VSQ_FMT_R5G5B5_LE)     ? " R5G5B5_LE"  : "",
           (m & VSQ_FMT_R5G6B5_LE)     ? " R5G6B5_LE"  : "",
           (m & VSQ_FMT_YUV420_PLANAR) ? " YUV420"     : "",
           (m & VSQ_FMT_NV12)          ? " NV12"       : "");
    printf("  max width %lu\n", (unsigned long)QueryVLayerAttr(scr, VSQ_MaxWidth));
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0, 0, 0 };
    struct RDArgs *rda;
    struct Screen *scr = NULL, *ownscr = NULL;
    struct Window *win = NULL;
    struct VLayerHandle *vh = NULL;
    STRPTR pubname = NULL;
    ULONG err = 0, features, frame = 0, srctype, minpitch;
    const char *fmtname;
    BOOL dbl, key, running = TRUE;
    int rc = RETURN_FAIL;

    rda = ReadArgs(TEMPLATE, args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "vlayerplay");
        return RETURN_ERROR;
    }

    CGXVideoBase = OpenLibrary("cgxvideo.library", 41);
    if (!CGXVideoBase)
    {
        printf("cannot open cgxvideo.library\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    if (args[ARG_SCREEN] && !strcasecmp((char *)args[ARG_SCREEN], "OWN"))
    {
        ownscr = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE,
            SA_Title, (IPTR)"vlayerplay", SA_ShowTitle, TRUE, TAG_DONE);
        scr = ownscr;
        if (!scr)
            printf("cannot open a screen\n");
    }
    else
    {
        pubname = (STRPTR)args[ARG_SCREEN];
        scr = LockPubScreen(pubname);
        if (!scr)
            printf("cannot lock public screen %s\n", pubname ? (char *)pubname : "(default)");
    }
    if (!scr)
        goto done;

    print_query(scr);
    features = QueryVLayerAttr(scr, VSQ_SupportedFeatures);
    dbl = args[ARG_DOUBLE] && (features & VSQ_FEAT_DOUBLEBUFFER);
    key = args[ARG_KEY] && (features & VSQ_FEAT_COLORKEYING);
    if (args[ARG_DOUBLE] && !dbl)
        printf("double buffering not available on this screen\n");
    if (args[ARG_KEY] && !key)
        printf("colour keying not available on this screen\n");

    if (args[ARG_RGB16])
    {
        srctype  = SRCFMT_RGB16PC;
        fmtname  = "RGB565";
        minpitch = SRC_W * 2;
    }
    else if (args[ARG_YUV420])
    {
        srctype  = SRCFMT_YCbCr420;
        fmtname  = "I420 planar (Y, U, V planes), BT.601 limited range";
        minpitch = SRC_W;
    }
    else if (args[ARG_NV12])
    {
        srctype  = SRCFMT_YCbCr420_NV12;
        fmtname  = "NV12 semi-planar (Y, interleaved UV plane), BT.601 limited range";
        minpitch = SRC_W;
    }
    else
    {
        srctype  = SRCFMT_YCbCr16;
        fmtname  = "YUYV, BT.601 limited range";
        minpitch = SRC_W * 2;
    }

    win = OpenWindowTags(NULL,
        ownscr ? WA_CustomScreen : WA_PubScreen, (IPTR)scr,
        WA_Left, 40, WA_Top, 40, WA_InnerWidth, SRC_W + 4, WA_InnerHeight, SRC_H + 4,
        WA_Title, (IPTR)"vlayerplay", WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE, WA_SizeGadget, TRUE, WA_SizeBBottom, TRUE,
        WA_MinWidth, 64, WA_MinHeight, 48, WA_MaxWidth, -1, WA_MaxHeight, -1,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE,
        WA_Activate, TRUE, TAG_DONE);
    if (!win)
    {
        printf("cannot open a window\n");
        goto done;
    }

    {
        struct TagItem tags[] =
        {
            { VOA_SrcType,      srctype        },
            { VOA_SrcWidth,     SRC_W          },
            { VOA_SrcHeight,    SRC_H          },
            { VOA_Error,        (IPTR)&err     },
            { VOA_DoubleBuffer, dbl            },
            { VOA_UseColorKey,  key            },
            { VOA_UseBackfill,  key            },
            { TAG_DONE,         0              }
        };

        vh = CreateVLayerHandleTagList(scr, tags);
    }
    if (!vh)
    {
        printf("CreateVLayerHandleTagList failed: error %lu%s\n", (unsigned long)err,
               err == VOERR_INVSCRMODE ? " (no overlay on this screen)" : "");
        rc = RETURN_WARN;
        goto done;
    }
    printf("layer %lux%lu %s created%s%s, modulo %lu\n", (unsigned long)SRC_W, (unsigned long)SRC_H, fmtname,
           dbl ? ", double buffered" : "", key ? ", colour keyed" : "",
           (unsigned long)GetVLayerAttr(vh, VOA_Modulo));
    if (key)
        printf("colour key 0x%06lx pen %ld\n", (unsigned long)GetVLayerAttr(vh, VOA_ColorKey),
               (long)GetVLayerAttr(vh, VOA_ColorKeyPen));

    {
        struct TagItem atags[] =
        {
            { VOA_LeftIndent,   2 },
            { VOA_TopIndent,    2 },
            { VOA_RightIndent,  2 },
            { VOA_BottomIndent, 2 },
            { TAG_DONE,         0 }
        };

        err = AttachVLayerTagList(vh, win, atags);
    }
    if (err)
    {
        printf("AttachVLayerTagList failed: %lu\n", (unsigned long)err);
        goto done;
    }
    printf("attached, close the window to quit\n");
    printf("expected picture (%s), 8 bars left to right:\n", fmtname);
    printf("  white yellow cyan green magenta red blue black%s\n",
           args[ARG_STATIC] ? " (STATIC: no movement)" : ", scrolling left with a dark line moving down");

    while (running)
    {
        struct IntuiMessage *msg;
        UBYTE *base;
        ULONG pitch;
        static UBYTE firstrow[64];
        static BOOL checkrow = FALSE;

        while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
        {
            if (msg->Class == IDCMP_CLOSEWINDOW)
                running = FALSE;
            ReplyMsg((struct Message *)msg);
        }
        if (!running)
            break;

        if (LockVLayer(vh))
        {
            base  = (UBYTE *)GetVLayerAttr(vh, VOA_BaseAddress);
            pitch = GetVLayerAttr(vh, VOA_Modulo);
            if (base && pitch >= minpitch)
            {
                if (args[ARG_RGB16])
                    fill_bars_rgb16(base, SRC_W, SRC_H, pitch, args[ARG_STATIC] ? 0 : frame);
                else if (args[ARG_YUV420])
                {
                    ULONG uoff = GetVLayerAttr(vh, VOA_BaseOffset1);
                    ULONG voff = GetVLayerAttr(vh, VOA_BaseOffset2);

                    if (frame == 0)
                        printf("I420 planes: modulo %lu, U at +%lu, V at +%lu (CGX rule: +%lu, +%lu)\n",
                               (unsigned long)pitch, (unsigned long)uoff, (unsigned long)voff,
                               (unsigned long)(pitch * SRC_H),
                               (unsigned long)(pitch * SRC_H + (pitch / 2) * (SRC_H / 2)));
                    if (uoff && voff)
                        fill_bars_i420(base, SRC_W, SRC_H, pitch, uoff, voff, args[ARG_STATIC] ? 0 : frame);
                    else if (frame == 0)
                        printf("no plane offsets reported, not drawing\n");
                }
                else if (args[ARG_NV12])
                {
                    ULONG uvoff = GetVLayerAttr(vh, VOA_BaseOffset1);

                    if (frame == 0)
                        printf("NV12 planes: modulo %lu, UV at +%lu (CGX rule: +%lu)\n",
                               (unsigned long)pitch, (unsigned long)uvoff,
                               (unsigned long)(pitch * SRC_H));
                    if (uvoff)
                        fill_bars_nv12(base, SRC_W, SRC_H, pitch, uvoff, args[ARG_STATIC] ? 0 : frame);
                    else if (frame == 0)
                        printf("no UV plane offset reported, not drawing\n");
                }
                else
                    fill_bars(base, SRC_W, SRC_H, pitch, args[ARG_STATIC] ? 0 : frame);
                if (frame == 0)
                {
                    /* the first row as written: read it back after the unlock below to check the mapping */
                    memcpy(firstrow, base, sizeof(firstrow));
                    checkrow = TRUE;
                }
            }
            UnlockVLayer(vh);
            if (checkrow && LockVLayer(vh))
            {
                UBYTE *again = (UBYTE *)GetVLayerAttr(vh, VOA_BaseAddress);

                /* with a single buffer this is the same memory: the write must have landed */
                if (again && !args[ARG_DOUBLE])
                    printf("readback of the first row after unlock: %s (%02x %02x %02x %02x ...)\n",
                           memcmp(again, firstrow, sizeof(firstrow)) ? "MISMATCH" : "ok",
                           again[0], again[1], again[2], again[3]);
                UnlockVLayer(vh);
                checkrow = FALSE;
            }
        }
        else
            printf("LockVLayer failed\n");

        if (dbl)
            SwapVLayerBuffer(vh);
        else
            WaitTOF();
        frame++;
    }

    printf("%lu frames\n", (unsigned long)frame);
    err = DetachVLayer(vh);
    printf("DetachVLayer: %lu\n", (unsigned long)err);
    rc = err ? RETURN_FAIL : RETURN_OK;

done:
    if (vh)
        DeleteVLayerHandle(vh);
    if (win)
        CloseWindow(win);
    if (ownscr)
        CloseScreen(ownscr);
    else if (scr)
        UnlockPubScreen(NULL, scr);
    CloseLibrary(CGXVideoBase);
    FreeArgs(rda);

    return rc;
}
