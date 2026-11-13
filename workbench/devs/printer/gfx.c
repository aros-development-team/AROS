/*
 * Copyright (C) 2012-2026, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Code related to Gfx printing
 */

#include <string.h>

#include <aros/debug.h>
#include <aros/printertag.h>

#include <exec/errors.h>

#include <proto/exec.h>
#include <proto/arossupport.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <cybergraphx/cybergraphics.h>
#include <graphics/displayinfo.h>
#include <graphics/view.h>

#include <devices/prtgfx.h>

#include <SDI/SDI_hook.h>

#include "printer_intern.h"

struct driverInfo {
    struct PrtInfo pi;
    struct PrinterData *di_PrinterData;
    BOOL di_8BitGuns;
    BOOL di_ConvertSource;
    BOOL di_FloydDithering;
    BOOL di_AntiAlias;
    BOOL di_ColorCorrection;
    BOOL di_NewColor;
    BOOL di_NoScaling;
    LONG di_ColorSize;
    LONG di_NumRows;
    WORD *di_FSError;       /* Floyd-Steinberg: two rows of errors, 4 per pixel */
    LONG di_FSFlip;
};

/* How the pens of a palettised source become colours */
#define PENS_NONE       0   /* not palettised: read as RGB */
#define PENS_PLAIN      1   /* through the colour map */
#define PENS_EHB        2   /* Extra Half-Brite */
#define PENS_HAM6       3
#define PENS_HAM8       4

/* A longword 0x00RRGGBB, as source hooks give them, in the byte order of this CPU */
#if AROS_BIG_ENDIAN
#define RECTFMT_ZRGB_NATIVE RECTFMT_0RGB32
#else
#define RECTFMT_ZRGB_NATIVE RECTFMT_BGR032
#endif

typedef LONG (*renderFunc )(SIPTR ct, LONG x, LONG y, LONG status);

#define RENDER(ct, x, y, status) ({ \
    D(bug("\tRENDER(%d): ct=%p x=%d y=%d\n", status, (APTR)ct, x, y)); \
    ((renderFunc)pi->pi_render)((SIPTR)(ct), (LONG)(x), (LONG)(y), (LONG)(status)); })

static void pg_ConvertSource(struct driverInfo *di, UBYTE *pdata, LONG width)
{
    struct PrinterData *pd = di->di_PrinterData;
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    LONG x;

    if (di->di_ColorSize < sizeof(union colorEntry)) {
        D(bug("\tdi->di_ColorSize<sizeof(union colorEntry)\n"));
        return;
    }

    for (x = 0; x < width; x++, pdata += di->di_ColorSize) {
        union colorEntry *ce = (APTR) pdata;
        UBYTE r, g, b, w;

        r = ce->colorByte[PCMRED];
        g = ce->colorByte[PCMGREEN];
        b = ce->colorByte[PCMBLUE];

        /* Find largest white*/
        w = (r > g) ? r : g;
        w = (b > w) ? b : w;

        if ((ped->ped_ColorClass & PCC_4COLOR)) {
            r = ~((w - r) * 255 / w);
            g = ~((w - g) * 255 / w);
            b = ~((w - b) * 255 / w);
        }

        if (!(ped->ped_ColorClass & PCC_ADDITIVE)) {
            r = ~r;
            g = ~g;
            b = ~b;
            w = ~w;
        }

        if (!di->di_8BitGuns) {
            r >>= 4;
            g >>= 4;
            b >>= 4;
            w >>= 4;
        }

        ce->colorByte[PCMRED] = r;
        ce->colorByte[PCMGREEN] = g;
        ce->colorByte[PCMBLUE] = b;
        ce->colorByte[PCMWHITE] = w;
    }
}


static void pg_ColorCorrection(struct driverInfo *di, UBYTE *pdata, LONG width)
{
    /*Nothing to do here*/
}

/* Floyd-Steinberg error diffusion for a driver that does not do it itself
 * (no PRTA_FloydDithering): every colour of every pixel becomes all or
 * nothing, so that whatever the driver compares it with - the dither
 * matrix, the threshold - sets the dot or leaves it. Ordered and halftone
 * dithering are the driver's, with the matrix in pi_dmatrix.
 */
static void pg_FloydDithering(struct driverInfo *di, UBYTE *pdata, LONG width)
{
    LONG channels = (di->di_ColorSize < 4) ? di->di_ColorSize : 4;
    LONG full = di->di_8BitGuns ? 255 : 15;
    LONG rowlen = (width + 2) * 4;
    WORD *cur, *next;
    LONG x, c;

    if (di->di_FSError == NULL)
        return;

    /* Errors are kept in 1/16; each row has a pixel of margin at both ends */
    cur  = di->di_FSError + (di->di_FSFlip ? rowlen : 0);
    next = di->di_FSError + (di->di_FSFlip ? 0 : rowlen);
    memset(next, 0, rowlen * sizeof(WORD));

    for (x = 0; x < width; x++, pdata += di->di_ColorSize) {
        for (c = 0; c < channels; c++) {
            LONG want = pdata[c] * 16 + cur[(x + 1) * 4 + c];
            LONG got = (want * 2 > full * 16) ? full : 0;
            LONG e = want - got * 16;

            pdata[c] = got;
            cur[(x + 2) * 4 + c]  += e * 7 / 16;
            next[x * 4 + c]       += e * 3 / 16;
            next[(x + 1) * 4 + c] += e * 5 / 16;
            next[(x + 2) * 4 + c] += e / 16;
        }
    }

    di->di_FSFlip ^= 1;
}

ULONG zrgbof(struct ColorMap *cm, ULONG index)
{
    UWORD hibits;
  
    /* For invalid entries, print white, not black
     */
    if (index >= cm->Count)
        return 0x00ffffff;

    hibits = cm->ColorTable[index];

    ULONG red8   = (hibits & 0x0f00) >> 4;
    ULONG green8 = (hibits & 0x00f0);
    ULONG blue8  = (hibits & 0x000f) << 4;

    if (cm->Type > COLORMAP_TYPE_V1_2) {
        UWORD lobits = cm->LowColorBits[index];

        red8   |= (lobits & 0x0f00) >> 8;
        green8 |= (lobits & 0x00f0) >> 4;
        blue8  |= (lobits & 0x000f);
    }

    return (red8 << 16) | (green8 << 8) | blue8;
}

/* One row of a palettised source as 0x00RRGGBB longwords, through the
 * request's colour map - which is what says what the pens mean, whatever
 * palette the bitmap may have of its own.
 *
 * A HAM pixel depends on the ones to its left, so such a row is always
 * read from the left edge; it starts from the background colour.
 */
static BOOL pg_ReadPenRow(struct IODRPReq *io, LONG mode, LONG y, UBYTE *pens, ULONG *buf)
{
    struct ColorMap *cm = io->io_ColorMap;
    LONG first = (mode == PENS_HAM6 || mode == PENS_HAM8) ? 0 : io->io_SrcX;
    LONG count = io->io_SrcX + io->io_SrcWidth - first;
    ULONG rgb = zrgbof(cm, 0);
    LONG x;

    if (ReadPixelLine8(io->io_RastPort, first, y, count, pens, NULL) != count)
        return FALSE;

    for (x = 0; x < count; x++) {
        UBYTE pen = pens[x];
        ULONG v;

        switch (mode) {
        case PENS_EHB:
            rgb = zrgbof(cm, pen & 31);
            if (pen & 32)
                rgb = (rgb >> 1) & 0x007f7f7f;
            break;
        case PENS_HAM6:
            v = (pen & 0x0f) * 0x11;
            switch ((pen >> 4) & 3) {
            case 0: rgb = zrgbof(cm, pen & 0x0f); break;
            case 1: rgb = (rgb & 0x00ffff00) | v; break;            /* blue */
            case 2: rgb = (rgb & 0x0000ffff) | (v << 16); break;    /* red */
            case 3: rgb = (rgb & 0x00ff00ff) | (v << 8); break;     /* green */
            }
            break;
        case PENS_HAM8:
            v = ((pen & 0x3f) << 2) | ((pen & 0x3f) >> 4);
            switch (pen >> 6) {
            case 0: rgb = zrgbof(cm, pen & 0x3f); break;
            case 1: rgb = (rgb & 0x00ffff00) | v; break;
            case 2: rgb = (rgb & 0x0000ffff) | (v << 16); break;
            case 3: rgb = (rgb & 0x00ff00ff) | (v << 8); break;
            }
            break;
        default:
            rgb = zrgbof(cm, pen);
            break;
        }
        if (first + x >= io->io_SrcX)
            *(buf++) = rgb;
    }

    return TRUE;
}

/* TurboPrint emulation helper hooks - convert from
 * native formats to 0RGB32 longwords
 */
DISPATCHER(TPSlice)
{
    struct DRPSourceMsg *drp = (struct DRPSourceMsg *)msg;
    struct IODRPReq *io = (struct IODRPReq *)obj;
    struct BitMap *tpbm = io->io_RastPort->BitMap;
    UBYTE *src = tpbm->Planes[0];
    ULONG *buf = drp->buf;
    UWORD bpr = tpbm->BytesPerRow;
    struct ColorMap *cm = io->io_ColorMap;
    struct TPExtIODRP *tp = (struct TPExtIODRP *)io->io_Modes;
    int w, h;
    int bpp;

    switch (tp->Mode) {
    case TPFMT_Chunky8:
        if (cm == NULL)
            return 0;
        bpp = 1;
        break;
    case TPFMT_RGB15:
    case TPFMT_BGR15:
    case TPFMT_RGB16:
    case TPFMT_BGR16:
        bpp = 2;
        break;
    case TPFMT_RGB24:
    case TPFMT_BGR24:
        bpp = 3;
        break;
    default:
        return 0;
    }

    src += (bpr * drp->y);
    for (h = 0; h < drp->height; h++, src += bpr) {
        UBYTE *sref = src + drp->x * bpp;
        for (w = 0; w < drp->width; w++ ) {
            ULONG zrgb;
            UWORD rgb15, rgb16;

            switch (tp->Mode) {
            case TPFMT_Chunky8:
                zrgb = zrgbof(cm, *(sref++));
                break;
            case TPFMT_RGB15:
                rgb15  = (*(sref++)) << 8;
                rgb15 |= *(sref++);
                goto rgb15;
            case TPFMT_BGR15:
                rgb15 = *(sref++);
                rgb15 |= (*(sref++)) << 8;
rgb15:
                zrgb = ((((rgb15 >> 10) & 0x1f)<<3) << 16) |
                       ((((rgb15 >>  5) & 0x1f)<<3) <<  8) |
                       ((((rgb15 >>  0) & 0x1f)<<3) <<  0);
                break;
            case TPFMT_RGB16:
                rgb16  = (*(sref++)) << 8;
                rgb16 |= *(sref++);
                goto rgb16;
            case TPFMT_BGR16:
                rgb16 = *(sref++);
                rgb16 |= (*(sref++)) << 8;
rgb16:
                zrgb = ((((rgb16 >> 11) & 0x1f)<<3) << 16) |
                       ((((rgb16 >>  5) & 0x3f)<<2) <<  8) |
                       ((((rgb16 >>  0) & 0x1f)<<3) <<  0);
                break;
            case TPFMT_RGB24:
                zrgb  = (*(sref++) << 16);
                zrgb |= (*(sref++) <<  8);
                zrgb |= (*(sref++) <<  0);
                break;
            case TPFMT_BGR24:
                zrgb  = (*(sref++) <<  0);
                zrgb |= (*(sref++) <<  8);
                zrgb |= (*(sref++) << 16);
                break;
            default:
                zrgb = 0x00ffffff;
                break;
            }

            *(buf++) = zrgb;
        }
    }

    return (buf - drp->buf);
}

MakeStaticHook(TPHook, TPSlice);

LONG  Printer_Gfx_DumpRPort(struct IODRPReq *io, struct TagItem *tags)
{
    struct PrinterData *pd = (struct PrinterData *)io->io_Device;
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    struct PrinterGfxPrefs *gfx = &pd->pd_PUnit->pu_Prefs.pp_Gfx;
    struct Hook *srcHook = NULL;
    struct TagItem *tag;
    struct BitMap *bm;
    LONG aspectXsrc = 1, aspectYsrc = 1;
    LONG aspectXdst = 1, aspectYdst = 1;
    LONG scaleXsrc, scaleXdst;
    LONG scaleYsrc, scaleYdst;
    LONG prnMarginLeft, prnMarginRight;
    LONG prnMarginTop, prnMarginBottom;
    LONG prnX;
    LONG prnW, prnH;
    LONG err;
    LONG special;
    LONG pens = PENS_NONE;
    BOOL ham = FALSE, ehb = FALSE;
    struct driverInfo di = {
        .di_PrinterData = pd,
        .di_ColorSize = sizeof(union colorEntry),
    };
    struct PrtInfo *pi = &di.pi;
    UBYTE const dmatrix[] = {
         1,  9,  3, 11,
        13,  5, 15,  7,
         4, 12,  2, 10,
        16,  8, 14,  6,
    };
    /* Halftone: the dots grow from the middle of the cell */
    UBYTE const hmatrix[] = {
        13,  7,  8, 14,
         6,  1,  2,  9,
         5,  4,  3, 10,
        16, 12, 11, 15,
    };

    D(bug("%s: io=%p, tags=%p\n", __func__, io, tags));

    pi->pi_render = (APTR)ped->ped_Render;
    if (!(ped->ped_PrinterClass & 1) || pi->pi_render == NULL) {
        /* Not graphics. */
        D(bug("\tNot a graphics printer (class = 0x%x)\n", ped->ped_PrinterClass));
        return PDERR_NOTGRAPHICS;
    }

    /* A request that names no density gets the one from the preferences */
    special = io->io_Special;
    if (!(special & SPECIAL_DENSITYMASK))
        special |= ((LONG)pd->pd_Preferences.PrintDensity << 8) & SPECIAL_DENSITYMASK;

    /* Set up density and printer dimensions */
    err = RENDER(io, special, 0, PRS_PREINIT);
    if (err < 0)
        return err;

    /* Get the source's aspect ratio */
    if (io->io_Command == PRD_TPEXTDUMPRPORT) {
        struct TPExtIODRP *tp = (APTR)io->io_Modes;
        if (tp == NULL)
            return PDERR_NOTGRAPHICS;
        aspectXsrc = tp->PixAspX;
        aspectYsrc = tp->PixAspY;
        D(bug("\ttp->Mode = 0x%04x\n", tp->Mode));
        ham = (tp->Mode == TPFMT_HAM);
        ehb = (tp->Mode == TPFMT_EHB);
        switch (tp->Mode) {
        case TPFMT_Chunky8:
        case TPFMT_BGR15:
        case TPFMT_BGR16:
        case TPFMT_BGR24:
        case TPFMT_RGB15:
        case TPFMT_RGB16:
        case TPFMT_RGB24:
            srcHook = &TPHook;
            break;
        case TPFMT_CyberGraphX:
        case TPFMT_BitPlanes:
        case TPFMT_HAM:
        case TPFMT_EHB:
            srcHook = NULL; /* AROS BitMap object - we can handle this */
            break;
        default:
            return PDERR_NOTGRAPHICS;
        }
    } else if (io->io_Modes != INVALID_ID) {
        struct DisplayInfo dpyinfo;

        if (GetDisplayInfoData(NULL, (APTR)&dpyinfo, sizeof(dpyinfo), DTAG_DISP, io->io_Modes)) {
            aspectXsrc = dpyinfo.Resolution.x;
            aspectYsrc = dpyinfo.Resolution.y;
            ham = (dpyinfo.PropertyFlags & DIPF_IS_HAM) ? TRUE : FALSE;
            ehb = (dpyinfo.PropertyFlags & DIPF_IS_EXTRAHALFBRITE) ? TRUE : FALSE;
        } else if ((io->io_Modes & ~(IPTR)0xffff) == 0) {
            /* a ViewPort's Modes, of no display this system knows */
            ham = (io->io_Modes & HAM) ? TRUE : FALSE;
            ehb = (io->io_Modes & EXTRA_HALFBRITE) ? TRUE : FALSE;
        }
    }

    /* Get the printer's aspect ratio */
    aspectXdst = ped->ped_XDotsInch;
    aspectYdst = ped->ped_YDotsInch;

    while ((tag = LibNextTagItem(&tags))) {
        switch (tag->ti_Tag) {
        case DRPA_SourceHook:
            srcHook = (struct Hook *)tag->ti_Data;
            break;
        case DRPA_AspectX:
            aspectXsrc = tag->ti_Data;
            break;
        case DRPA_AspectY:
            aspectYsrc = tag->ti_Data;
            break;
        default:
            break;
        }
    }
        
    if ((pd->pd_SegmentData->ps_Version >= 44) &&
        (ped->ped_PrinterClass & PPCF_EXTENDED)) {
        tags = ped->ped_TagList;
        while ((tag = LibNextTagItem(&tags))) {
            switch (tag->ti_Tag) {
            case PRTA_8BitGuns:
                di.di_8BitGuns = (BOOL)tag->ti_Data;
                break;
            case PRTA_ConvertSource:
                di.di_ConvertSource = (BOOL)tag->ti_Data;
                break;
            case PRTA_FloydDithering:
                di.di_FloydDithering = (BOOL)tag->ti_Data;
                break;
            case PRTA_AntiAlias:
                di.di_AntiAlias = (BOOL)tag->ti_Data;
                break;
            case PRTA_ColorCorrection:
                di.di_ColorCorrection = (BOOL)tag->ti_Data;
                break;
            case PRTA_NoIO:
                /* Handled in driver.c */
                break;
            case PRTA_NewColor:
                di.di_NewColor = (BOOL)tag->ti_Data;
                break;
            case PRTA_ColorSize:
                di.di_ColorSize = (ULONG)tag->ti_Data;
                break;
            case PRTA_NoScaling:
                di.di_NoScaling = (BOOL)tag->ti_Data;
                break;
            case PRTA_DitherNames:
            case PRTA_ShadingNames:
            case PRTA_ColorCorrect:
            case PRTA_DensityInfo:
                /* Handled in :Prefs/Printer */
                break;
            case PRTA_LeftBorder:
            case PRTA_TopBorder:
            case PRTA_MixBWColor:
            case PRTA_Preferences:
                /* Advice for applications */
                break;
            default:
                break;
            }
        }
    }

    if (di.di_NewColor) {
        di.di_ConvertSource = TRUE;
        di.di_FloydDithering = TRUE;
        di.di_AntiAlias = TRUE;
        di.di_ColorCorrection = TRUE;
    }

    if (di.di_NoScaling) {
        di.di_FloydDithering = TRUE;
        di.di_AntiAlias = TRUE;
    }

    if (di.di_8BitGuns) {
        di.di_FloydDithering = TRUE;
    }

    if (di.di_ColorSize < 3) {
        D(bug("\tPRTA_ColorSize was %d - illegal!\n", di.di_ColorSize));
        return PDERR_BADDIMENSION;
    }

    if (di.di_ColorSize < sizeof(union colorEntry) && !di.di_ConvertSource
        && !di.di_ColorCorrection) {
        D(bug("\tPRTA_ColorSize of %d is illegal without PRTA_ConvertSource and PRTA_ColorCorrection!\n", di.di_ColorSize));
        return PDERR_BADDIMENSION;
    }

    prnMarginLeft = gfx->pg_PrintXOffset;
    prnMarginRight = 0;
    prnMarginTop = gfx->pg_PrintYOffset;
    prnMarginBottom = 0;

    prnX = 0;
    prnW = io->io_DestCols ? io->io_DestCols : io->io_SrcWidth;
    prnH = io->io_DestRows ? io->io_DestRows : io->io_SrcHeight;

    di.di_NumRows = ped->ped_NumRows; /* Rows/Stripe */

    if (io->io_Special == 0) {
        /* From the OS 3.9 autodocs for printer.device... */
        if (prnW == 0 && prnH > 0) {
            prnW = (ped->ped_MaxXDots - (prnMarginLeft + prnMarginRight));
        } else if (prnW == 0 && prnH == 0) {
            prnW = (ped->ped_MaxXDots - (prnMarginLeft + prnMarginRight));
            prnH = prnW * aspectYsrc / aspectXsrc;
        } else if (prnW > 0 && prnH == 0) {
            prnH = prnW * aspectYsrc / aspectXsrc;
        } else if (prnW < 0 && prnH > 0) {
            prnW = io->io_SrcWidth * (-prnW) / prnH;
            prnH = prnW * aspectYsrc / aspectXsrc;
        }
    }

    if (io->io_Special & SPECIAL_MILCOLS) {
        prnW = io->io_DestCols * ped->ped_XDotsInch / 1000;
    }
    if (io->io_Special & SPECIAL_MILROWS) {
        prnH = io->io_DestRows * ped->ped_YDotsInch / 1000;
    }
    /* The following math relies on the fact that at
     * ped_Max?Dots is limited to 65535 pixels:
     * At even 1200dpi, 65535 pixels is over 100 inches!
     */
    if (io->io_Special & SPECIAL_FRACCOLS) {
        prnW = (ped->ped_MaxXDots * (io->io_DestCols >> 16)) >> 16;
    }
    if (io->io_Special & SPECIAL_FRACROWS) {
        prnH = (ped->ped_MaxYDots * (io->io_DestRows >> 16)) >> 16;
    }
    /* Full page width (minus the margins) */
    if (io->io_Special & SPECIAL_FULLCOLS) {
        prnW = ped->ped_MaxXDots - (prnMarginLeft + prnMarginRight);
    }
    if (io->io_Special & SPECIAL_FULLROWS) {
        prnH = ped->ped_MaxYDots - (prnMarginTop + prnMarginBottom);
    }
    if (io->io_Special & SPECIAL_ASPECT) {
        prnH = prnH * aspectYdst / aspectXdst;
    }

    /* Autoshrink to maximum page size */
    if (prnW > (ped->ped_MaxXDots - (prnMarginLeft + prnMarginRight ))) {
        LONG delta = prnW - (ped->ped_MaxXDots - (prnMarginLeft + prnMarginRight ));
        prnH = prnH - delta * prnH / prnW;
        prnW = prnW - delta;
    }

    if (prnH > (ped->ped_MaxYDots - (prnMarginTop + prnMarginBottom ))) {
        LONG delta = prnH - (ped->ped_MaxYDots - (prnMarginTop + prnMarginBottom ));
        prnW = prnW - delta * prnW / prnH;
        prnH = prnH - delta;
    }

    /* Centering. Only across the paper: where on it the dump starts is
     * the driver's business, which knows where the paper is.
     */
    if (io->io_Special & SPECIAL_CENTER) {
        prnX = prnMarginLeft + (ped->ped_MaxXDots - (prnMarginLeft + prnMarginRight ) - prnW) / 2;
    } else {
        prnX = prnMarginLeft;
    }

    D(bug("\tAspect: %dx%d %d:%d => %dx%d %d:%d\n",
                io->io_SrcWidth, io->io_SrcHeight,
                aspectXsrc, aspectYsrc,
                prnW, prnH,
                aspectXdst, aspectYdst));

    /* Scaling calculations. */
    scaleXsrc = io->io_SrcWidth;
    scaleYsrc = io->io_SrcHeight;
    scaleXdst = prnW;
    scaleYdst = prnH;

    while (scaleXsrc > 168383 || scaleXdst > 168383) {
        scaleXsrc >>= 1;
        scaleXdst >>= 1;
        if (scaleXsrc == 0 || scaleXdst == 0) {
            D(bug("\tCan't scale X from %dx%d to %dx%d\n", io->io_SrcWidth, io->io_SrcHeight, prnW, prnH));
            return PDERR_BADDIMENSION;
        }
    }

    while (scaleYsrc > 168383 || scaleYdst > 168383) {
        scaleYsrc >>= 1;
        scaleYdst >>= 1;
        if (scaleYsrc == 0 || scaleYdst == 0) {
            D(bug("\tCan't scale Y from %dx%d to %dx%d\n", io->io_SrcWidth, io->io_SrcHeight, prnW, prnH));
            return PDERR_BADDIMENSION;
        }
    }


    prnW = ScalerDiv(io->io_SrcWidth, scaleXdst, scaleXsrc);
    prnH = ScalerDiv(io->io_SrcHeight, scaleYdst, scaleYsrc);

    D(bug("\tScaling %dx%d (%d:%d) => %dx%d (%d:%d)\n",
                io->io_SrcWidth, io->io_SrcHeight, scaleXsrc, scaleYsrc,
                prnW, prnH, scaleXdst, scaleYdst));

    io->io_DestCols = prnW;
    io->io_DestRows = prnH;


    /* If nothing to print, we're done! */
    if (io->io_Special & SPECIAL_NOPRINT) {
        D(bug("\tOh, SPECIAL_NOPRINT. Done.\n"));
        return 0;
    }

    /* A bitmap of up to 8 planes holds pens, and the request's colour map
     * says what they are. HAM and Extra Half-Brite are ways to read pens.
     */
    if (srcHook == NULL) {
        LONG depth;

        if (io->io_RastPort == NULL || io->io_RastPort->BitMap == NULL)
            return PDERR_NOTGRAPHICS;
        depth = GetBitMapAttr(io->io_RastPort->BitMap, BMA_DEPTH);
        if (depth <= 8) {
            if (io->io_ColorMap != NULL) {
                if (ham && depth >= 7)
                    pens = PENS_HAM8;
                else if (ham && depth >= 5)
                    pens = PENS_HAM6;
                else if (ehb && depth == 6)
                    pens = PENS_EHB;
                else
                    pens = PENS_PLAIN;
            } else if (ham || ehb) {
                D(bug("\tHAM or EHB without a colour map\n"));
                return PDERR_NOTGRAPHICS;
            }
        }
    }

    if (di.di_NumRows < 1)
        di.di_NumRows = 1;

    /* Set up the PrtInfo structure */
    pi->pi_rp = io->io_RastPort;
    pi->pi_ScaleX = NULL; /* New di.di_s should *not* be using this */
    pi->pi_dmatrix = (UBYTE *)(((pd->pd_Preferences.PrintFlags & DITHERING_MASK) == HALFTONE_DITHERING) ?
                               hmatrix : dmatrix);
    pi->pi_width = prnW;
    pi->pi_height = prnH;
    pi->pi_xpos = prnX;
    pi->pi_threshold = pd->pd_Preferences.PrintThreshold;
    pi->pi_PrefsFlags = pd->pd_Preferences.PrintFlags;
    pi->pi_special = special;
    pi->pi_SourceHook = srcHook;

   /* Initialize page for printing */
    if (0 == (err = RENDER(io, prnW, prnH, PRS_INIT))) {
        APTR pdata;
        struct BitMap *src_bm = NULL;
        struct BitMap *tmp_bm = NULL;   /* the source, if we had to make one */
        LONG srcX = io->io_SrcX, srcY = io->io_SrcY;

        if (di.di_NoScaling) {
            prnW = io->io_SrcWidth;
            prnH = io->io_SrcHeight;
            scaleXsrc = scaleXdst = 1;
            scaleYsrc = scaleYdst = 1;
        }

        if (srcHook || pens != PENS_NONE) {
            /* The source comes a row at a time, as 0x00RRGGBB longwords -
             * from the caller's hook, or from the pens of a palettised
             * bitmap. Collect it in a bitmap of our own.
             */
            ULONG *rgb;
            UBYTE *penrow = NULL;
            /* ReadPixelLine8() wants a multiple of 16 pixels of room */
            LONG pensize = ((io->io_SrcX + io->io_SrcWidth + 15) >> 4) << 4;
            struct RastPort src_rp;
            InitRastPort(&src_rp);

            /* In case we fail.. */
            err = PDERR_INTERNALMEMORY;
            if (pens != PENS_NONE)
                penrow = AllocMem(pensize, MEMF_PUBLIC);
            if ((rgb = AllocMem(io->io_SrcWidth * 4, MEMF_PUBLIC)) &&
                (penrow != NULL || pens == PENS_NONE)) {
                if ((tmp_bm = AllocBitMap(io->io_SrcWidth, io->io_SrcHeight, 24, BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_RGB24) , io->io_RastPort->BitMap))) {
                    struct DRPSourceMsg msg;
                    LONG row;

                    src_rp.BitMap = tmp_bm;
                    err = 0;

                    msg.x = io->io_SrcX;
                    msg.y = io->io_SrcY;
                    msg.width = io->io_SrcWidth;
                    msg.buf = rgb;

                    for (row = 0; row < io->io_SrcHeight; row++, msg.y++) {
                        /* Collect the next source row */
                        if (srcHook) {
                            msg.height = 1;
                            CallHookA(srcHook, io, &msg);
                        } else if (!pg_ReadPenRow(io, pens, msg.y, penrow, rgb)) {
                            D(bug("\tCan't read the pens of row %d\n", msg.y));
                            err = PDERR_NOTGRAPHICS;
                            break;
                        }
                        /* Transfer to source bitmap */
                        WritePixelArray(rgb, 0, 0, io->io_SrcWidth * 4, &src_rp, 0, row, io->io_SrcWidth, 1, RECTFMT_ZRGB_NATIVE);
                    }
                    if (err == 0) {
                        /* it holds just the part that is printed */
                        src_bm = tmp_bm;
                        srcX = srcY = 0;
                    }
                } else {
                    D(bug("\tCan't allocate bitmap to hold the source rows (%d x %d)\n", io->io_SrcWidth, io->io_SrcHeight));
                }
            } else {
                D(bug("\tCan't allocate a %dx4 row to hold the source rows\n", io->io_SrcWidth));
            }
            if (rgb)
                FreeMem(rgb, io->io_SrcWidth * 4);
            if (penrow)
                FreeMem(penrow, pensize);
        } else {
            src_bm = io->io_RastPort->BitMap;
        }

        if (src_bm) {
            struct RastPort rp;
            InitRastPort(&rp);
            if ((pdata = AllocMem(prnW * di.di_ColorSize, MEMF_PUBLIC))) {
                pi->pi_ColorInt = pdata;
                pi->pi_ColorIntSize = prnW * di.di_ColorSize;

                if ((bm = AllocBitMap(prnW, prnH, GetBitMapAttr(src_bm, BMA_DEPTH), 0, src_bm))) {
                    /* Render it ourselves */
                    struct BitScaleArgs bsa = {
                        .bsa_SrcBitMap = src_bm,
                        .bsa_SrcX = srcX,
                        .bsa_SrcY = srcY,
                        .bsa_SrcWidth = io->io_SrcWidth,
                        .bsa_SrcHeight = io->io_SrcHeight,
                        .bsa_XSrcFactor = scaleXsrc,
                        .bsa_XDestFactor = scaleXdst,
                        .bsa_YSrcFactor = scaleYsrc,
                        .bsa_YDestFactor = scaleYdst,
                        .bsa_DestBitMap = bm,
                    };
                    LONG row;
                    LONG rleft = prnH;
                    LONG fssize = 0;

                    BitMapScale(&bsa);
                    rp.BitMap = bm;

                    /* If we made a temporary source bitmap, we no longer need it.
                     */
                    if (tmp_bm) {
                        FreeBitMap(tmp_bm);
                        tmp_bm = NULL;
                    }

                    /* Floyd-Steinberg, when the preferences ask for it and
                     * the driver leaves it to us; without the memory for
                     * it the dump is printed undithered.
                     */
                    if (!di.di_FloydDithering &&
                        (pd->pd_Preferences.PrintFlags & FLOYD_DITHERING)) {
                        fssize = (prnW + 2) * 4 * 2 * sizeof(WORD);
                        di.di_FSError = AllocMem(fssize, MEMF_PUBLIC | MEMF_CLEAR);
                    }

                    for (row = 0; row < prnH && err == 0; rleft -= di.di_NumRows) {
                        LONG rows = (rleft > di.di_NumRows) ? di.di_NumRows : rleft;
                        int i;
                        for (i =0; i < rows; i++, row++) {
                            if (Printer_Aborted(pd)) {
                                err = IOERR_ABORTED;
                                break;
                            }

                            /* A row that cannot be read is not printed as
                             * whatever the buffer happens to hold.
                             */
                            if (ReadPixelArray(pdata, 0, 0, prnW * di.di_ColorSize,
                                           &rp, 0, row, prnW, 1,
                                           di.di_ColorSize == 3 ?
                                             RECTFMT_BGR24 :
                                             RECTFMT_BGR032) != prnW) {
                                D(bug("\tCan't read row %d of the scaled bitmap\n", row));
                                err = PDERR_NOTGRAPHICS;
                                break;
                            }

                            /* Convert from RGB to printer color space */
                            if (di.di_ConvertSource)
                                RENDER(pdata, prnW, 1, PRS_CONVERT);
                            else
                                pg_ConvertSource(&di, pdata, prnW);

                            /* Apply printer color space corrections */
                            if (di.di_ColorCorrection)
                                RENDER(pdata, prnW, 1, PRS_CORRECT);
                            else
                                pg_ColorCorrection(&di, pdata, prnW);

                            if (di.di_FSError)
                                pg_FloydDithering(&di, pdata, prnW);

                            /* y is the row of the dump, 0 to height - 1 */
                            err = RENDER(pi, 0, row, PRS_TRANSFER);
                            if (err != 0)
                                break;
                        }
                        if (err == 0)
                            err = RENDER(0, 0, rows, PRS_FLUSH);
                    }
                    if (di.di_FSError) {
                        FreeMem(di.di_FSError, fssize);
                        di.di_FSError = NULL;
                    }
                    FreeBitMap(bm);
                } else {
                    D(bug("\tCan't allocate a %dx%d bitmap for the scaled data\n", prnW, prnH));
                    err = PDERR_INTERNALMEMORY;
                }
                FreeMem(pdata, prnW * di.di_ColorSize);
            } else {
                D(bug("\tCan't allocate a %d x %d byte transfer row\n", prnW, di.di_ColorSize));
                err = PDERR_INTERNALMEMORY;
            }
        } else {
            D(bug("\tCan't find nor synthesize a source bitmap\n"));
        }

        if (tmp_bm)
            FreeBitMap(tmp_bm);

        {
            LONG closeerr = RENDER((SIPTR)err, special, 0, PRS_CLOSE);
            if (err == 0)
                err = closeerr;
        }
    }

    return err;
}
