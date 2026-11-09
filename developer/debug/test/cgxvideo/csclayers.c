/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    csclayers: video layers in different YCbCr encodings at once. Three
    windows show the same RGB colour bars, each encoded in its own colour
    space and range (BT.601 limited, BT.709 limited, BT.709 full). With
    the right matrix per layer all bars look the same; a layer converted
    with another's matrix shows other colours or contrast.

    A display with fewer matrices than encodings must leave a layer it
    cannot convert unplaced rather than show it wrong.

    TWO/S - only BT.601 limited and BT.709 full
*/

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <proto/cgxvideo.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <stdio.h>

struct Library *CGXVideoBase;

#define SRC_W 256
#define SRC_H 192

struct enc
{
    const char *name;
    ULONG       space, range;
    double      kr, kb;
};

static const struct enc encs[] =
{
    { "BT.601 limited", VOCS_BT601, VOCR_LIMITED, 0.299,  0.114  },
    { "BT.709 limited", VOCS_BT709, VOCR_LIMITED, 0.2126, 0.0722 },
    { "BT.709 full",    VOCS_BT709, VOCR_FULL,    0.2126, 0.0722 }
};

static UBYTE clamp8(double v)
{
    return (v < 0) ? 0 : (v > 255) ? 255 : (UBYTE)(v + 0.5);
}

/* R, G, B (0..1) to Y, Cb, Cr in the encoding */
static void rgb2ycc(const struct enc *e, double r, double g, double b, UBYTE out[3])
{
    double y = e->kr * r + (1.0 - e->kr - e->kb) * g + e->kb * b;
    double cb = (b - y) / (2.0 * (1.0 - e->kb));
    double cr = (r - y) / (2.0 * (1.0 - e->kr));

    if (e->range == VOCR_FULL)
    {
        out[0] = clamp8(255.0 * y);
        out[1] = clamp8(128.0 + 255.0 * cb);
        out[2] = clamp8(128.0 + 255.0 * cr);
    }
    else
    {
        out[0] = clamp8(16.0 + 219.0 * y);
        out[1] = clamp8(128.0 + 224.0 * cb);
        out[2] = clamp8(128.0 + 224.0 * cr);
    }
}

/* Eight bars (white yellow cyan green magenta red blue black), NV12:
 * the Y plane, then interleaved Cb, Cr at uvoff with the same pitch */
static void fill(const struct enc *e, UBYTE *base, ULONG pitch, ULONG uvoff)
{
    static const UBYTE rgb[8][3] =
    {
        { 1, 1, 1 }, { 1, 1, 0 }, { 0, 1, 1 }, { 0, 1, 0 },
        { 1, 0, 1 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 0, 0 }
    };
    UBYTE ycc[8][3];
    ULONG i, x, y;

    for (i = 0; i < 8; i++)
        rgb2ycc(e, rgb[i][0] * 0.75, rgb[i][1] * 0.75, rgb[i][2] * 0.75, ycc[i]);

    for (y = 0; y < SRC_H; y++)
    {
        UBYTE *row = base + y * pitch;

        for (x = 0; x < SRC_W; x++)
            row[x] = ycc[(x * 8) / SRC_W][0];
    }
    for (y = 0; y < SRC_H / 2; y++)
    {
        UBYTE *row = base + uvoff + y * pitch;

        for (x = 0; x < SRC_W; x += 2)
        {
            row[x + 0] = ycc[(x * 8) / SRC_W][1];
            row[x + 1] = ycc[(x * 8) / SRC_W][2];
        }
    }
}

int main(void)
{
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    struct Screen *scr = NULL;
    struct Window *win[3] = { NULL, NULL, NULL };
    APTR vh[3] = { NULL, NULL, NULL };
    const struct enc *use[3];
    ULONG n, i, err, sigs = 0;
    int rc = RETURN_FAIL;

    rda = ReadArgs("TWO/S", args, NULL);
    if (rda && args[0])
    {
        use[0] = &encs[0];
        use[1] = &encs[2];
        n = 2;
    }
    else
    {
        for (i = 0; i < 3; i++)
            use[i] = &encs[i];
        n = 3;
    }
    if (rda)
        FreeArgs(rda);

    if (!(CGXVideoBase = OpenLibrary("cgxvideo.library", 43)))
    {
        printf("cannot open cgxvideo.library\n");
        return RETURN_FAIL;
    }
    if (!(scr = LockPubScreen(NULL)))
        goto done;

    for (i = 0; i < n; i++)
    {
        struct TagItem tags[] =
        {
            { VOA_SrcType,    SRCFMT_YCbCr420_NV12 },
            { VOA_SrcWidth,   SRC_W          },
            { VOA_SrcHeight,  SRC_H          },
            { VOA_ColorSpace, use[i]->space  },
            { VOA_ColorRange, use[i]->range  },
            { VOA_Error,      (IPTR)&err     },
            { TAG_DONE,       0              }
        };

        win[i] = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr,
            WA_Left, 20 + i * (SRC_W + 30), WA_Top, 60,
            WA_InnerWidth, SRC_W, WA_InnerHeight, SRC_H,
            WA_Title, (IPTR)use[i]->name, WA_DragBar, TRUE, WA_DepthGadget, TRUE,
            WA_CloseGadget, TRUE, WA_IDCMP, IDCMP_CLOSEWINDOW, TAG_DONE);
        if (!win[i])
        {
            printf("cannot open a window\n");
            goto done;
        }
        sigs |= 1UL << win[i]->UserPort->mp_SigBit;

        if (!(vh[i] = CreateVLayerHandleTagList(scr, tags)))
        {
            printf("%s: CreateVLayerHandleTagList failed, error %lu\n", use[i]->name, (unsigned long)err);
            rc = RETURN_WARN;
            goto done;
        }
        if (LockVLayer(vh[i]))
        {
            fill(use[i], (UBYTE *)GetVLayerAttr(vh[i], VOA_BaseAddress), GetVLayerAttr(vh[i], VOA_Modulo),
                 GetVLayerAttr(vh[i], VOA_BaseOffset1));
            UnlockVLayer(vh[i]);
        }
        if ((err = AttachVLayerTagList(vh[i], win[i], NULL)))
        {
            printf("%s: AttachVLayerTagList failed: %lu\n", use[i]->name, (unsigned long)err);
            goto done;
        }
    }

    printf("%lu layers. Every one shown should have the same bars:\n", (unsigned long)n);
    printf("  white yellow cyan green magenta red blue black, at 75%%\n");
    printf("Other colours or contrast in one = it got another layer's matrix.\n");
    printf("Close a window to quit.\n");

    Wait(sigs | SIGBREAKF_CTRL_C);
    rc = RETURN_OK;

done:
    for (i = 0; i < 3; i++)
    {
        if (vh[i])
        {
            DetachVLayer(vh[i]);
            DeleteVLayerHandle(vh[i]);
        }
        if (win[i])
            CloseWindow(win[i]);
    }
    if (scr)
        UnlockPubScreen(NULL, scr);
    CloseLibrary(CGXVideoBase);
    return rc;
}
