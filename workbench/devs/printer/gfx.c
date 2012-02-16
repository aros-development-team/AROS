/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Code related to Gfx printing
 */

#include <string.h>

#include <aros/debug.h>
#include <aros/printertag.h>

#include <proto/exec.h>
#include <proto/arossupport.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <cybergraphx/cybergraphics.h>

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
};

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

static void pg_FloydDithering(struct driverInfo *di, UBYTE *pdata, LONG width)
{
    /* Nothing done here for now.
     *
     * In theory, we should do Ordered and Floyd-Steinberg dithering here.
     */
}

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
    LONG prnX, prnY;
    LONG prnW, prnH;
    LONG err;
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

    D(bug("%s: io=%p, tags=%p\n", __func__, io, tags));

    pi->pi_render = (APTR)ped->ped_Render;
    if (!(ped->ped_PrinterClass & 1) || pi->pi_render == NULL) {
        /* Not graphics. */
        D(bug("\tNot a graphics printer (class = 0x%x)\n", ped->ped_PrinterClass));
        return PDERR_NOTGRAPHICS;
    }

    /* Set up density and printer dimensions */
    err = RENDER(io, io->io_Special, 0, PRS_PREINIT);
    if (err < 0)
        return err;

    /* Get the source's aspect ratio */
    if (io->io_Modes != INVALID_ID) {
        struct DisplayInfo dpyinfo;

        if (GetDisplayInfoData(NULL, (APTR)&dpyinfo, sizeof(dpyinfo), DTAG_DISP, io->io_Modes)) {
            aspectXsrc = dpyinfo.Resolution.x;
            aspectYsrc = dpyinfo.Resolution.y;
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
    prnY = 0;
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

    /* Centering */
    if (io->io_Special & SPECIAL_CENTER) {
        prnX = prnMarginLeft + (ped->ped_MaxXDots - (prnMarginLeft + prnMarginRight ) - prnW) / 2;
        prnY = prnMarginTop + (ped->ped_MaxYDots - (prnMarginTop  + prnMarginBottom) - prnH) / 2;
    } else {
        prnX = prnMarginLeft;
        prnY = prnMarginTop;
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

    /* Set up the PrtInfo structure */
    pi->pi_rp = io->io_RastPort;
    pi->pi_ScaleX = NULL; /* New di.di_s should *not* be using this */
    pi->pi_dmatrix = (UBYTE *) dmatrix;
    pi->pi_width = prnW;
    pi->pi_height = prnH;
    pi->pi_xpos = prnX;
    pi->pi_threshold = pd->pd_Preferences.PrintThreshold;
    pi->pi_special = io->io_Special;
    pi->pi_SourceHook = srcHook;

   /* Initialize page for printing */
    if (0 == (err = RENDER(io, prnW, prnH, PRS_INIT))) {
        APTR pdata;
        struct BitMap *src_bm = NULL;

        if (di.di_NoScaling) {
            prnW = io->io_SrcWidth;
            prnH = io->io_SrcHeight;
            scaleXsrc = scaleXdst = 1;
            scaleYsrc = scaleYdst = 1;
        }

        /* Allocate a row for 24-bit RGB color information */
        if (srcHook) {
            APTR pdata;
            struct RastPort src_rp;
            InitRastPort(&src_rp);

            /* In case we fail.. */
            err = PDERR_INTERNALMEMORY;
            if ((pdata = AllocMem(io->io_SrcWidth * 4, MEMF_PUBLIC))) {
                if ((src_bm = AllocBitMap(io->io_SrcWidth, io->io_SrcHeight, 24, BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_RGB24) , io->io_RastPort->BitMap))) {
                    struct DRPSourceMsg msg;
                    LONG row;

                    src_rp.BitMap = src_bm;
                    
                    msg.x = io->io_SrcX;
                    msg.y = io->io_SrcY;
                    msg.width = io->io_SrcWidth;
                    msg.buf = (APTR)pdata;

                    for (row = 0; row < io->io_SrcHeight; row++, msg.y++) {
                        /* Collect the next source row */
                        msg.height = 1;
                        CallHookA(srcHook, io, &msg);
                        /* Transfer to source bitmap */
                        WritePixelArray(pdata, 0, 0, io->io_SrcWidth * 4, &src_rp, 0, row, io->io_SrcWidth, 1, RECTFMT_0RGB32);
                    }
                } else {
                    D(bug("\tCan't allocate bitmap to hold srcHook data (%d x %d)\n", io->io_SrcWidth, io->io_SrcHeight));
                }
                FreeMem(pdata, io->io_SrcWidth * 4);
            } else {
                D(bug("\tCan't allocate a %dx4 row to hold srcHook data\n", io->io_SrcWidth));
            }
        } else {
            src_bm = io->io_RastPort->BitMap;
        }

        if (src_bm) {
            struct RastPort rp;
            InitRastPort(&rp);
            if ((pdata = AllocMem(prnW * di.di_ColorSize, MEMF_PUBLIC))) {
                pi->pi_ColorInt = pdata;
                pi->pi_ColorIntSize = prnW * di.di_ColorSize;

                if ((bm = AllocBitMap(prnW, prnH, -1, 0, io->io_RastPort->BitMap))) {
                    /* Render it ourselves */
                    struct BitScaleArgs bsa = {
                        .bsa_SrcBitMap = io->io_RastPort->BitMap,
                        .bsa_SrcX = io->io_SrcX,
                        .bsa_SrcY = io->io_SrcY,
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

                    BitMapScale(&bsa);
                    rp.BitMap = bm;

                    /* If we make a temporary source bitmap, we no longer need it.
                     */
                    if (srcHook && src_bm) {
                        FreeBitMap(src_bm);
                    }

                    for (row = 0; row < prnH; rleft -= di.di_NumRows) {
                        LONG rows = (rleft > di.di_NumRows) ? di.di_NumRows : rleft;
                        int i;
                        for (i =0; i < rows; i++, row++) {
                            ReadPixelArray(pdata, 0, 0, prnW * di.di_ColorSize,
                                           &rp, 0, row, prnW, 1,
                                           di.di_ColorSize == 3 ?
                                             RECTFMT_BGR24 :
                                             RECTFMT_BGR032);

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

                            if (!di.di_FloydDithering)
                                pg_FloydDithering(&di, pdata, prnW);

                            RENDER(pi, 0, prnY + row, PRS_TRANSFER);
                        }
                        RENDER(0, 0, rows, PRS_FLUSH);
                    }
                    FreeBitMap(bm);
                } else {
                    D(bug("\tCan't allocate a %dx%d bitmap for the scaled data\n", prnW, prnH));
                }
                FreeMem(pdata, prnW * di.di_ColorSize);
            } else {
                D(bug("\tCan't allocate a %d x %d byte transfer row\n", prnW, di.di_ColorSize));
            }
        } else {
            D(bug("\tCan't find nor synthesize a source bitmap\n"));
        }

        RENDER(err, io->io_Special, 0, PRS_CLOSE);
    }

    return err;
}


