/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    underlay: a client surface below its screen (CRF_UNDERLAY_OK), seen
    through the screen's alpha. Opens a quiet screen with alpha, writes its
    pixels with a known alpha (opaque, with a transparent hole; inside it a
    white bar ramping from transparent to opaque and a red square with
    alpha 0) and attaches colour bars, a little larger than the hole, to
    the display's compositor as a client surface.

    First 3 s above the screen: the bars cover the hole and the margin round
    it (the plane's own pixels). Then below it: the bars show only inside
    the hole, the bar over them fades from bars (left) to white (right), the
    red square is not seen and nothing of the plane shows outside the hole.
    A red square tinting the bars means the screen blends premultiplied; a
    solid one, that it does not blend. The compositor decides where the
    screen goes (the primary, or a plane of its own in front of the
    Workbench); the surface hook reports the plane and whether it is below.
    Ends after SECONDS (default 15) or on Ctrl-C.
*/

#define __OOP_NOATTRBASES__

#include <aros/asmcall.h>
#include <cybergraphx/cybergraphics.h>
#include <dos/dos.h>
#include <hidd/gfx.h>
#include <intuition/screens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/oop.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <string.h>

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddDisplayAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;

#define TEMPLATE "SECONDS/N"

/* Byte offsets of R, G and B in a 32-bit pixel of the screen's format; A is byte 3 */
static int ro, go, bo;

static BOOL channels(IPTR stdpf)
{
    switch (stdpf)
    {
    case vHidd_StdPixFmt_BGR032:
    case vHidd_StdPixFmt_BGRA32:
        bo = 0; go = 1; ro = 2;
        return TRUE;
    case vHidd_StdPixFmt_RGB032:
    case vHidd_StdPixFmt_RGBA32:
        ro = 0; go = 1; bo = 2;
        return TRUE;
    }
    return FALSE;
}

static void put(UBYTE *p, UBYTE r, UBYTE g, UBYTE b, UBYTE a)
{
    p[ro] = r; p[go] = g; p[bo] = b; p[3] = a;
}

/* The screen: opaque, a transparent hole, and in it a white bar ramping in */
static void fill_screen(UBYTE *base, ULONG pitch, LONG w, LONG h, struct Rectangle *hole)
{
    LONG x, y;
    LONG barh = (hole->MaxY - hole->MinY + 1) / 6;
    LONG bary = hole->MaxY - 2 * barh;

    for (y = 0; y < h; y++)
    {
        UBYTE *row = base + y * pitch;

        for (x = 0; x < w; x++)
        {
            UBYTE *p = row + x * 4;
            BOOL in = (x >= hole->MinX) && (x <= hole->MaxX) && (y >= hole->MinY) && (y <= hole->MaxY);

            if (!in)
            {
                BOOL grid = ((x % 64) == 0) || ((y % 64) == 0);
                UBYTE v = (UBYTE)(40 + (y * 120) / h);

                put(p, grid ? 200 : 0, grid ? 200 : v / 2, grid ? 200 : v, 0xff);
            }
            else if ((x < hole->MinX + barh) && (y < hole->MinY + barh))
                put(p, 0xff, 0, 0, 0);      /* colour, but alpha 0 */
            else if ((y >= bary) && (y < bary + barh))
            {
                /* straight alpha: white, alpha 0 at the left edge to 255 at the right */
                UBYTE a = (UBYTE)(((x - hole->MinX) * 255) / (hole->MaxX - hole->MinX));

                put(p, 0xff, 0xff, 0xff, a);
            }
            else
                put(p, 0, 0, 0, 0);
        }
    }
}

/* The plane: eight vertical colour bars */
static void fill_bars(UBYTE *base, ULONG pitch, LONG w, LONG h)
{
    static const UBYTE bars[8][3] =
    {
        { 255, 255, 255 }, { 255, 255, 0 }, { 0, 255, 255 }, { 0, 255, 0 },
        { 255, 0, 255 },   { 255, 0, 0 },   { 0, 0, 255 },   { 40, 40, 40 }
    };
    LONG x, y;

    for (y = 0; y < h; y++)
    {
        UBYTE *row = base + y * pitch;

        for (x = 0; x < w; x++)
        {
            const UBYTE *c = bars[(x * 8) / w];

            put(row + x * 4, c[0], c[1], c[2], 0xff);
        }
    }
}

/* What the compositor told the surface: set by the hook, read by main() */
static volatile ULONG gained, lost, below;

AROS_UFH3(static IPTR, surface_hook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, display, A2),
    AROS_UFHA(struct HIDD_SurfaceHookMsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    if (msg && (msg->Type == vHidd_SurfaceHook_PlaneGained))
    {
        gained++;
        below = (msg->Flags & SHF_UNDERLAY) ? 1 : 0;
    }
    else if (msg && (msg->Type == vHidd_SurfaceHook_PlaneLost))
        lost++;
    return 0;

    AROS_USERFUNC_EXIT
}

/* The alpha format of a 32-bit screen pixel format, or -1 */
static LONG alpha_pixfmt(ULONG pf)
{
    switch (pf)
    {
    case PIXFMT_BGR032: case PIXFMT_BGRA32: return PIXFMT_BGRA32;
    case PIXFMT_RGB032: case PIXFMT_RGBA32: return PIXFMT_RGBA32;
    case PIXFMT_0RGB32: case PIXFMT_ARGB32: return PIXFMT_ARGB32;
    }
    return -1;
}

static void report(const char *when)
{
    printf("%s: %s", when, gained ? "on a plane" : "no plane");
    if (gained)
        printf(", %s the screen", below ? "below" : "above");
    if (lost)
        printf(" (lost it %lu times)", (unsigned long)lost);
    printf("\n");
}

int main(void)
{
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    struct Screen *wb, *scr = NULL;
    OOP_Object *bmobj, *display = NULL, *pf = NULL, *tbm = NULL;
    struct HIDD_CompositeRequest req;
    struct Hook hook;
    struct Rectangle hole;
    IPTR stdpf = 0, w = 0, h = 0, amask = 0;
    LONG apf = -1, seconds = 15, i;
    BOOL attached = FALSE;
    int rc = RETURN_FAIL;
    APTR lock;
    UBYTE *base = NULL;
    ULONG pitch = 0;

    rda = ReadArgs(TEMPLATE, args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "underlay");
        return RETURN_FAIL;
    }
    if (args[0])
        seconds = *(LONG *)args[0];

    HiddBitMapAttrBase  = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    HiddDisplayAttrBase = OOP_ObtainAttrBase(IID_Hidd_Display);
    HiddPixFmtAttrBase  = OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    if (!HiddBitMapAttrBase || !HiddDisplayAttrBase || !HiddPixFmtAttrBase)
    {
        printf("no gfx.hidd attribute bases\n");
        goto done;
    }

    /* Like the Workbench, with alpha */
    if ((wb = LockPubScreen(NULL)))
    {
        apf = alpha_pixfmt(GetCyberMapAttr(wb->RastPort.BitMap, CYBRMATTR_PIXFMT));
        UnlockPubScreen(NULL, wb);
    }
    if (apf < 0)
    {
        printf("needs a 32-bit Workbench\n");
        rc = RETURN_WARN;
        goto done;
    }
    scr = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE, SA_PixelFormat, apf, SA_Quiet, TRUE,
                         SA_ShowTitle, FALSE, SA_Type, CUSTOMSCREEN, SA_Title, (IPTR)"underlay", TAG_DONE);
    if (!scr)
    {
        printf("cannot open a screen\n");
        goto done;
    }
    bmobj = HIDD_BM_OBJ(scr->RastPort.BitMap);
    OOP_GetAttr(bmobj, aHidd_BitMap_Display, (IPTR *)&display);
    OOP_GetAttr(bmobj, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    if (pf)
    {
        OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
        OOP_GetAttr(pf, aHidd_PixFmt_AlphaMask, &amask);
    }
    OOP_GetAttr(bmobj, aHidd_BitMap_Width, &w);
    OOP_GetAttr(bmobj, aHidd_BitMap_Height, &h);
    if (!display || !channels(stdpf))
    {
        printf("needs a 32-bit RGB screen (pixfmt %lu)\n", (unsigned long)stdpf);
        goto done;
    }
    if (!amask)
    {
        printf("the screen got no alpha (pixfmt %lu): nothing can show below it\n", (unsigned long)stdpf);
        rc = RETURN_WARN;
        goto done;
    }

    /* The screen's pixels */
    hole.MinX = w / 4;
    hole.MinY = h / 4;
    hole.MaxX = (w * 3) / 4 - 1;
    hole.MaxY = (h * 3) / 4 - 1;
    lock = LockBitMapTags(scr->RastPort.BitMap, LBMI_BASEADDRESS, (IPTR)&base,
                          LBMI_BYTESPERROW, (IPTR)&pitch, TAG_DONE);
    if (!lock || !base)
    {
        printf("cannot lock the screen bitmap\n");
        goto done;
    }
    fill_screen(base, pitch, w, h, &hole);
    UnLockBitMap(lock);

    /* The surface's bitmap, the screen's format */
    {
        struct TagItem bmtags[] =
        {
            { aHidd_BitMap_Scanout,   TRUE         },
            { aHidd_BitMap_Width,     w / 2 + 64   },
            { aHidd_BitMap_Height,    h / 2 + 64   },
            { aHidd_BitMap_StdPixFmt, stdpf        },
            { aHidd_BitMap_Friend,    (IPTR)bmobj  },
            { TAG_DONE,               0            }
        };

        tbm = HIDD_Display_CreateObject(display, OOP_FindClass(CLID_Hidd_BitMap), bmtags);
    }
    if (!tbm || !HIDD_BM_LockBuffer(tbm, 0, &base, &pitch, NULL))
    {
        printf("no scan-out bitmap for the surface\n");
        goto done;
    }
    fill_bars(base, pitch, w / 2 + 64, h / 2 + 64);
    HIDD_BM_UnlockBuffer(tbm, 0, NULL);

    /* 32 pixels larger than the hole all round, in screen coordinates */
    memset(&req, 0, sizeof(req));
    req.BitMap    = tbm;
    req.Dest.MinX = hole.MinX - 32;
    req.Dest.MinY = hole.MinY - 32;
    req.Dest.MaxX = hole.MaxX + 32;
    req.Dest.MaxY = hole.MaxY + 32;
    req.Src.MaxX  = req.Dest.MaxX - req.Dest.MinX;
    req.Src.MaxY  = req.Dest.MaxY - req.Dest.MinY;
    req.Opacity   = 0xff;
    req.AlphaMode = vHidd_CompAlpha_None;
    req.Flags     = CRF_PLANE_REQUIRED;
    memset(&hook, 0, sizeof(hook));
    hook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(surface_hook);

    /* Above first, to see the plane's own pixels */
    if (!HIDD_Display_AttachSurface(display, bmobj, &req, &hook))
    {
        printf("the compositor refused the surface\n");
        goto done;
    }
    attached = TRUE;
    Delay(150);
    report("above for 3 s");

    /* Then below: the screen's alpha shows it */
    gained = lost = below = 0;
    req.Flags |= CRF_UNDERLAY_OK;
    HIDD_Display_UpdateSurface(display, &req);
    Delay(50);
    report("with CRF_UNDERLAY_OK");
    printf("%ld s or Ctrl-C\n", (long)seconds);
    for (i = 0; (i < seconds * 10) && !(SetSignal(0, 0) & SIGBREAKF_CTRL_C); i++)
        Delay(5);

    rc = (gained && below) ? RETURN_OK : RETURN_WARN;

done:
    if (attached)
        HIDD_Display_DetachSurface(display, &req);
    if (tbm)
        OOP_DisposeObject(tbm);
    if (scr)
        CloseScreen(scr);
    if (HiddPixFmtAttrBase)
        OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    if (HiddDisplayAttrBase)
        OOP_ReleaseAttrBase(IID_Hidd_Display);
    if (HiddBitMapAttrBase)
        OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    FreeArgs(rda);
    return rc;
}
