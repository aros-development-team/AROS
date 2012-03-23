/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <graphics/gfxmacros.h>
#include <proto/icon.h>

#include "icon_intern.h"

/* Bitmap scaling */
static BOOL scaleToResolution(ULONG SrcWidth, ULONG SrcHeight,
                   UWORD SrcResX, UWORD SrcResY,
                   ULONG *DstWidth, ULONG *DstHeight,
                   UWORD DstResX, UWORD DstResY,
                   ULONG *ScaleXSrc, ULONG *ScaleYSrc,
                   ULONG *ScaleXDst, ULONG *ScaleYDst,
                   struct IconBase *IconBase);

static BOOL scaleToBounds(ULONG SrcWidth, ULONG SrcHeight,
                   UWORD MaxWidth, UWORD MaxHeight,
                   ULONG *DstWidth, ULONG *DstHeight,
                   ULONG *ScaleXSrc, ULONG *ScaleYSrc,
                   ULONG *ScaleXDst, ULONG *ScaleYDst,
                   struct IconBase *IconBase);

/* ARGB image scaling */
static void ScaleRect(ULONG *Target, const ULONG *Source, int SrcWidth, int SrcHeight, int TgtWidth, int TgtHeight);

/*****************************************************************************

    NAME */

    AROS_LH3(BOOL, LayoutIconA,

/*  SYNOPSIS */
        AROS_LHA(struct DiskObject *, icon,   A0),
        AROS_LHA(struct Screen *,     screen, A1),
        AROS_LHA(struct TagItem *,    tags,   A2),

/*  LOCATION */
        struct IconBase *, IconBase, 32, Icon)

/*  FUNCTION
	Adapt a palette-mapped icon for display.
	
    INPUTS

    RESULT

    NOTES
	Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Default source DPI is the Amiga PAL DPI */
    ULONG width, height;
    ULONG scaleXsrc = 1, scaleYsrc = 1, scaleXdst = 1, scaleYdst = 1;
    ULONG mutualexclude = (ULONG)icon->do_Gadget.MutualExclude;
    ULONG scalebox;
    struct NativeIcon *ni;
    struct RastPort rp;
    struct DrawInfo *dri;
    struct ColorMap *cm;
    BOOL ret;
    int i, j;
    const ULONG bflags = BMF_CLEAR;
    ni = GetNativeIcon(icon, LB(IconBase));
    if (!ni)
        return TRUE;

    if (ni->ni_Width <= 0)
        ni->ni_Width = ni->ni_DiskObject.do_Gadget.Width;
    if (ni->ni_Height <= 0)
        ni->ni_Height = ni->ni_DiskObject.do_Gadget.Height;

    D(bug("[%s] Icon %p, Screen %p, %xx%x icon\n", __func__, icon, screen, ni->ni_Width, ni->ni_Height));

    /* Already mapped to this screen?
     */
    if (screen == ni->ni_Screen)
        return TRUE;

    for (i = 0; i < 2; i++) {
        struct NativeIconImage *image = &ni->ni_Image[i];

        if (image->BitMap) {
            FreeBitMap(image->BitMap);
            image->BitMap = NULL;
        }

        if (image->ARGBMap && image->ARGBMap != image->ARGB) {
            FreeVec(image->ARGBMap);
        }

        if (image->Pen) {
            for (j = 0; j < image->Pens; j++) {
                ReleasePen(ni->ni_Screen->ViewPort.ColorMap, image->Pen[j]);
            }
            FreeVec(image->Pen);
            image->Pen = NULL;
        }

        /* Remove the synthesized BitMask */
        if (image->BitMask) {
            FreeVec(image->BitMask);
            image->BitMask = NULL;
         }
    }

    ni->ni_Screen = NULL;

    if (screen == NULL)
        return TRUE;

    dri = GetScreenDrawInfo(screen);
    if (dri == NULL)
        return FALSE;

    /* Look up the DrawInfo Pens we will need for the
     * layout of the border and frame.
     */
    for (i = 0; i < NUMDRIPENS; i++) {
        if (i < dri->dri_NumPens)
            ni->ni_Pens[i] = dri->dri_Pens[i];
        else
            ni->ni_Pens[i] = dri->dri_Pens[DETAILPEN];
    }

    cm = screen->ViewPort.ColorMap;

    /* NOTE: The ARGB data (if present) will not need
     *       any layout work.
     */
    D(bug("%s: Screen %p, Depth %d, ColorMap %d\n", __func__, screen, dri->dri_Depth, cm ? cm->Count : -1));

    ret = TRUE;

    /* Calculate the scaling factors
     */
    if (ni->ni_Face.Width && ni->ni_Face.Height) {
        width = ni->ni_Face.Width;
        height= ni->ni_Face.Height;
    } else {
        width = icon->do_Gadget.Width;
        height = icon->do_Gadget.Height;
    }

    if (ni->ni_ScaleBox == ICON_SCALEBOX_DEFAULT)
        scalebox = LB(IconBase)->ib_ScaleBox;
    else
        scalebox = ni->ni_ScaleBox;

    ni->ni_Width = width;
    ni->ni_Height = height;

    /* Are we rescaling dynamically? */
    if (scalebox == ICON_SCALEBOX_AUTOSCALE) {
        UBYTE tpdX = 0, tpdY = 0;

        /* Check for a magic MutualExlcude value
         * that encodes Tick-Per-Dot information.
         * MutalExclude of 0xffffffff is not valid.
         */
        if ((mutualexclude != 0xffffffff) && (mutualexclude & (1 << 31))) {
            /* tpd information is in the lower 16 bits */
            tpdX = (mutualexclude >>  8) & 0xff;
            tpdY = (mutualexclude >>  0) & 0xff;
        }

        if (tpdX && tpdY) {
            scaleToResolution(width, height, tpdX, tpdY,
                         &ni->ni_Width, &ni->ni_Height, dri->dri_Resolution.X, dri->dri_Resolution.Y,
                         &scaleXsrc, &scaleYsrc, &scaleXdst, &scaleYdst,
                         IconBase);
        }
        D(bug("%s: Icon tpd (%d:%d), Screen tpd (%d:%d)\n", __func__,
                    tpdX, tpdY,  dri->dri_Resolution.X, dri->dri_Resolution.Y));

    } else {
        WORD MaxWidth, MaxHeight;

        UNPACK_ICON_SCALEBOX(scalebox, MaxWidth, MaxHeight);

        scaleToBounds(width, height, MaxWidth, MaxHeight,
                      &ni->ni_Width, &ni->ni_Height,
                      &scaleXsrc, &scaleYsrc, &scaleXdst, &scaleYdst,
                      IconBase);
    }

    for (i = 0; i < 2; i++) {
        struct NativeIconImage *image = &ni->ni_Image[i];
        struct TagItem pentags[] = {
            { OBP_Precision, IconBase->ib_Precision },
            { OBP_FailIfBad, FALSE },
            { TAG_MORE, (IPTR)tags },
        };
        UBYTE *idata;
        ULONG x;
        UWORD bmdepth;

        bmdepth = GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH);

        /* If we can use ARGB data, then use it! */
        D(bug("[%s] Screen depth is %d\n", __func__, bmdepth));
        if ((bmdepth > 8) && CyberGfxBase) {
            FetchIconARGB(icon, i);
            if (image->ARGB) {
                if (width != ni->ni_Width || height != ni->ni_Height) {
                    if ((image->ARGBMap = AllocVec(ni->ni_Width * ni->ni_Height * sizeof(ULONG), MEMF_PUBLIC))) {
                        D(bug("[%s] ARGB scaling\n"));
                        ScaleRect(image->ARGBMap, image->ARGB, width, height, ni->ni_Width, ni->ni_Height);
                        continue;
                    }
                } else {
                    image->ARGBMap = (APTR)image->ARGB;
                }
            }
            if (!image->ARGBMap) {
                D(bug("[%s] No ARGB image\n"));
            }
        }

        /* Allocate a bitmap, which is a 'friend' of the screen */
        image->BitMap = AllocBitMap(width, height, dri->dri_Depth, bflags, screen->RastPort.BitMap);
        if (image->BitMap == NULL) {
            SetIoErr(ERROR_NO_FREE_STORE);
            ret = FALSE;
            goto exit;
        }

        FetchIconImage(icon, i);

        if (!image->ImageData) {
            struct Image *gi = NULL;
            ULONG state;
            BOOL flood = FALSE;

            if (i == 1 && (icon->do_Gadget.Flags & GFLG_GADGHIMAGE)) {
                gi = icon->do_Gadget.SelectRender;
            } else if (icon->do_Gadget.Flags & GFLG_GADGIMAGE) {
                gi = icon->do_Gadget.GadgetRender;
            }

            if (gi == NULL)
                continue;

            if (i == 1 && (icon->do_Gadget.Flags & GFLG_GADGHCOMP))
                state = IDS_SELECTED;
            else
                state = IDS_NORMAL;

            if (i == 1 && (icon->do_Gadget.Flags & GFLG_GADGBACKFILL)) {
                state = IDS_SELECTED;
                flood = TRUE;
            }

            InitRastPort(&rp);
            if (!flood) {
                rp.BitMap = image->BitMap;
                DrawImageState(&rp, gi, 0, 0, state, dri);
            } else {
                /* Create a bitmap with a 1 pixel border,
                 * fill it with the inverse color,
                 * draw the inverse image into it,
                 * then flood-fill the border with color 0.
                 *
                 * Finally, copy the final image to the
                 * destination bitmap.
                 */
                struct BitMap *bm;
                if ((bm = AllocBitMap(gi->Width+2, gi->Height+2, gi->Depth, BMF_CLEAR, NULL))) {
                    PLANEPTR trbuf;

                    rp.BitMap = bm;

                    if ((trbuf = AllocRaster(gi->Width+2, gi->Height+2))) {
                        struct TmpRas tr;
                        InitTmpRas(&tr, trbuf, RASSIZE(gi->Width+2, gi->Height+2));
                        rp.TmpRas = &tr;
                        SetAPen(&rp, (1 << gi->Depth)-1);
                        RectFill(&rp, 0, 0, gi->Width+1, gi->Height+1);
                        DrawImageState(&rp, gi, 1, 1, state, dri);
                        SetAPen(&rp, 0);
                        Flood(&rp, 1, 0, 0);
                        BltBitMap(bm, 1, 1, image->BitMap, 0, 0, gi->Width, gi->Height, 0xc0, ~0, NULL);
                        FreeRaster(trbuf, gi->Width+2, gi->Height+2);
                    }
                    FreeBitMap(bm);
                }
            }

            goto rescale;
        }

        /* Palettized image processing */
        if (!image->Pen)
            image->Pen = AllocVec(image->Pens * sizeof(image->Pen[0]), MEMF_PUBLIC | MEMF_CLEAR);

        if (!image->Pen) {
            SetIoErr(ERROR_NO_FREE_STORE);
            ret = FALSE;
            goto exit;
        }

        /* Get the needed colormap entries. */
        for (j = 0; j < image->Pens; j++) {
            ULONG r,g,b;
            LONG pen;
            /* CHECKME: So, uh, how does one accuarately
             *          convert 8 bit RGB to 32 bit RBG?
             */
            r = image->Palette[j].red   << 24;
            g = image->Palette[j].green << 24;
            b = image->Palette[j].blue  << 24;
            pen = ObtainBestPenA(cm, r, g, b, pentags);
            image->Pen[j] = (UBYTE)pen;
        }

        /* Draw the selected state into the screen's pens
         *
         * We take the risk of yet another memory allocation
         * so that we can use WriteChunkyPixels(), which is
         * GOBS faster than WritePixel().
         */
        idata = AllocVec(height * width, MEMF_ANY);
        if (idata == NULL) {
            FreeBitMap(image->BitMap);
            image->BitMap = NULL;
            SetIoErr(ERROR_NO_FREE_STORE);
            ret = FALSE;
            goto exit;
        }
        CopyMem(image->ImageData, idata, height * width);
        for (x = 0; x < (height * width); x++) {
            idata[x] = image->Pen[image->ImageData[x]];
        }
        InitRastPort(&rp);
        rp.BitMap = image->BitMap;
        WriteChunkyPixels(&rp, 0, 0, width - 1, height - 1,
                          idata, width);
        FreeVec(idata);

        /* Synthesize a bitmask for transparentcolor icons */
        D(bug("[%s] TransparentColor %d\n", __func__, image->TransparentColor));
        if (image->TransparentColor >= 0) {
            int x, y;
            UBYTE *row;
            CONST UBYTE *img;
            UWORD bpr = image->BitMap->BytesPerRow;
            image->BitMask = AllocVec(bpr * height + 4, MEMF_PUBLIC | MEMF_CHIP | MEMF_CLEAR);
            if (!image->BitMask) {
                SetIoErr(ERROR_NO_FREE_STORE);
                return FALSE;
            }

            img = image->ImageData;
            row = image->BitMask;
#ifdef __mc68000 /* AGA support */
            row = (APTR)(((IPTR)row + 7) & ~7);
#endif
            for (y = 0; y < height; y++, row += bpr) {
                for (x = 0; x < width; x++, img++) {
                    if ((*img != image->TransparentColor)) {
                        row[x>>3] |= 1 << (7 - (x & 7));
                    }
                }
            }
        }

rescale:
        if (width != ni->ni_Width || height != ni->ni_Height) {
            struct BitMap *bm = AllocBitMap(ni->ni_Width, ni->ni_Height, dri->dri_Depth, bflags, image->BitMap);

            D(bug("%s: Rescaling from %dx%d to %dx%d\n", __func__,
                        width, height, ni->ni_Width, ni->ni_Height));

            if (bm) {
                struct BitScaleArgs bsa = {
                    .bsa_SrcBitMap = image->BitMap,
                    .bsa_SrcX = 0,
                    .bsa_SrcY = 0,
                    .bsa_SrcWidth = width,
                    .bsa_SrcHeight = height,
                    .bsa_XSrcFactor = scaleXsrc,
                    .bsa_XDestFactor = scaleXdst,
                    .bsa_YSrcFactor = scaleYsrc,
                    .bsa_YDestFactor = scaleYdst,
                    .bsa_DestBitMap = bm,
                };
                BitMapScale(&bsa);

                if (image->BitMask) {
                    struct BitMap src, dst;
                    PLANEPTR dst_mask;
                    src.BytesPerRow = image->BitMap->BytesPerRow;
                    src.Rows = height;
                    src.Flags = 0;
                    src.Depth = 1;
                    src.Planes[0] = image->BitMask;
#ifdef __mc68000 /* AGA support */
                    src.Planes[0] = (APTR)(((IPTR)src.Planes[0] + 7) & ~7);
#endif
                    dst.BytesPerRow = bm->BytesPerRow;
                    dst.Rows = ni->ni_Height;
                    dst.Flags = 0;
                    dst.Depth = 1;
                    dst_mask = AllocVec(dst.BytesPerRow * dst.Rows + 4, MEMF_PUBLIC | MEMF_CHIP | MEMF_CLEAR);
                    dst.Planes[0] = dst_mask;
                    if (dst.Planes[0]) {
#ifdef __mc68000 /* AGA support */
                        dst.Planes[0] = (APTR)(((IPTR)dst.Planes[0] + 7) & ~7);
#endif
                        bsa.bsa_SrcBitMap = &src;
                        bsa.bsa_DestBitMap = &dst;
                        bsa.bsa_SrcX = 0,
                        bsa.bsa_SrcY = 0,
                        bsa.bsa_SrcWidth = width,
                        bsa.bsa_SrcHeight = height,
                        bsa.bsa_XSrcFactor = scaleXsrc,
                        bsa.bsa_XDestFactor = scaleXdst,
                        bsa.bsa_YSrcFactor = scaleYsrc,
                        bsa.bsa_YDestFactor = scaleYdst,

                        BitMapScale(&bsa);
                        FreeVec(image->BitMask);
                        image->BitMask = dst_mask;
                    } else {
                        FreeVec(dst_mask);
                        FreeBitMap(bm);
                        continue;
                    }
                }

                FreeBitMap(image->BitMap);
                image->BitMap = bm;
            }
        }
    }

    ni->ni_Screen = screen;

exit:
    FreeScreenDrawInfo(screen, dri);

    if (ret == TRUE)
        SetIoErr(0);
    
    return ret;
    
    AROS_LIBFUNC_EXIT
} /* LayoutIconA() */


static BOOL scaleToBounds(ULONG SrcWidth, ULONG SrcHeight,
                   UWORD MaxWidth, UWORD MaxHeight,
                   ULONG *DstWidth, ULONG *DstHeight,
                   ULONG *ScaleXSrc, ULONG *ScaleYSrc,
                   ULONG *ScaleXDst, ULONG *ScaleYDst,
                   struct IconBase *IconBase)
{
    ULONG scaleXsrc, scaleYsrc, scaleXdst, scaleYdst;

    if (MaxWidth <= 0 || MaxHeight <= 0) 
        return FALSE;

    /* Scaling calculations
     */
    scaleXsrc = SrcWidth;
    scaleYsrc = SrcHeight;
    scaleXdst = MaxWidth;
    scaleYdst = SrcHeight * MaxWidth / SrcWidth;

    if (scaleYdst > MaxHeight) {
        LONG delta = scaleYdst - MaxHeight;
        scaleXdst -= delta * SrcWidth / SrcHeight;
        scaleYdst -= delta;
    }

    while (scaleXsrc > 168383 || scaleXdst > 168383) {
        scaleXsrc >>= 1;
        scaleXdst >>= 1;
        if (scaleXsrc == 0 || scaleXdst == 0) {
            D(bug("\tCan't scale X from %dx%d to %dx%d\n", scaleXsrc, scaleYsrc, scaleXdst, scaleYdst));
            return FALSE;
        }
    }

    while (scaleYsrc > 168383 || scaleYdst > 168383) {
        scaleYsrc >>= 1;
        scaleYdst >>= 1;
        if (scaleYsrc == 0 || scaleYdst == 0) {
            D(bug("\tCan't scale Y from %dx%d to %dx%d\n", scaleXsrc, scaleYsrc, scaleXdst, scaleYdst));
            return FALSE;
        }
    }

    *DstWidth = ScalerDiv(SrcWidth, scaleXdst, scaleXsrc);
    *DstHeight = ScalerDiv(SrcHeight, scaleYdst, scaleYsrc);

    *ScaleXSrc = scaleXsrc;
    *ScaleYSrc = scaleYsrc;

    *ScaleXDst = scaleXdst;
    *ScaleYDst = scaleYdst;

    D(bug("[%s] Scale icon %dx%d to box %dx%d => %dx%d\n", __func__, SrcWidth, SrcHeight, MaxWidth, MaxHeight, *DstWidth, *DstHeight));

    return TRUE;
}

static BOOL scaleToResolution(ULONG SrcWidth, ULONG SrcHeight,
                   UWORD SrcResX, UWORD SrcResY,
                   ULONG *DstWidth, ULONG *DstHeight,
                   UWORD DstResX, UWORD DstResY,
                   ULONG *ScaleXSrc, ULONG *ScaleYSrc,
                   ULONG *ScaleXDst, ULONG *ScaleYDst,
                   struct IconBase *IconBase)
{
    ULONG scaleXsrc, scaleYsrc, scaleXdst, scaleYdst;

    /* Scaling calculations
     * Remember: 'res' is in 'ticks', which is inversely
     *           related to display DPI.
     */
    scaleXsrc = SrcWidth;
    scaleYsrc = SrcHeight;
    scaleXdst = SrcWidth  * SrcResX / DstResX;
    scaleYdst = SrcHeight * SrcResY / DstResY;

    while (scaleXsrc > 168383 || scaleXdst > 168383) {
        scaleXsrc >>= 1;
        scaleXdst >>= 1;
        if (scaleXsrc == 0 || scaleXdst == 0) {
            D(bug("\tCan't scale X from %dx%d to %dx%d\n", scaleXsrc, scaleYsrc, scaleXdst, scaleYdst));
            return FALSE;
        }
    }

    while (scaleYsrc > 168383 || scaleYdst > 168383) {
        scaleYsrc >>= 1;
        scaleYdst >>= 1;
        if (scaleYsrc == 0 || scaleYdst == 0) {
            D(bug("\tCan't scale Y from %dx%d to %dx%d\n", scaleXsrc, scaleYsrc, scaleXdst, scaleYdst));
            return FALSE;
        }
    }

    *DstWidth = ScalerDiv(SrcWidth, scaleXdst, scaleXsrc);
    *DstHeight = ScalerDiv(SrcHeight, scaleYdst, scaleYsrc);

    *ScaleXSrc = scaleXsrc;
    *ScaleYSrc = scaleYsrc;

    *ScaleXDst = scaleXdst;
    *ScaleYDst = scaleYdst;

    D(bug("[%s] Scale icon %dx%d => %dx%d\n", __func__, SrcWidth, SrcHeight, *DstWidth, *DstHeight));

    return TRUE;
}

/* From 'Image Scaling With Bresenham', Dr. Dobbs  Journal, May 1, 2002
 */
static inline void ScaleLine(ULONG *Target, const ULONG *Source, int SrcWidth, int TgtWidth)
{
    int NumPixels = TgtWidth;
    int IntPart = SrcWidth / TgtWidth;
    int FracPart = SrcWidth % TgtWidth;
    int E = 0;
    while (NumPixels-- > 0) {
        *(Target++) = *Source;
        Source += IntPart;
        E += FracPart;
        if (E >= TgtWidth) {
            E -= TgtWidth;
            Source++;
        }
    }
}

static void ScaleRect(ULONG *Target, const ULONG *Source, int SrcWidth, int SrcHeight, int TgtWidth, int TgtHeight)
{
    int NumPixels = TgtHeight;
    int IntPart = (SrcHeight / TgtHeight) * SrcWidth;
    int FractPart = SrcHeight % TgtHeight;
    int E = 0;
    const ULONG *PrevSource = NULL;
    while (NumPixels-- > 0) {
        if (Source == PrevSource) {
            CopyMem(&Target[-TgtWidth], Target, TgtWidth*sizeof(*Target));
        } else {
            ScaleLine(Target, Source, SrcWidth, TgtWidth);
            PrevSource = Source;
        }
        Target += TgtWidth;
        Source += IntPart;
        E += FractPart;
        if (E >= TgtHeight) {
            E -= TgtHeight;
            Source += SrcWidth;
        }
    }
}
