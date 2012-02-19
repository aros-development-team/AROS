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

    /* Set up the width and height if needed */
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

    for (i = 0; i < 2; i++) {
        struct NativeIconImage *image = &ni->ni_Image[i];
        struct TagItem pentags[] = {
            { OBP_Precision, IconBase->ib_Precision },
            { OBP_FailIfBad, FALSE },
            { TAG_MORE, (IPTR)tags },
        };
        UBYTE *idata;
        ULONG x;

        /* Allocate a bitmap, which is a 'friend' of the screen */
        image->BitMap = AllocBitMap(ni->ni_Width, ni->ni_Height, dri->dri_Depth, bflags, screen->RastPort.BitMap);
        if (image->BitMap == NULL) {
            SetIoErr(ERROR_NO_FREE_STORE);
            ret = FALSE;
            goto exit;
        }

        /* This should have been loaded earlier */
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

            continue;
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
        idata = AllocVec(ni->ni_Height * ni->ni_Width, MEMF_ANY);
        if (idata == NULL) {
            FreeBitMap(image->BitMap);
            image->BitMap = NULL;
            SetIoErr(ERROR_NO_FREE_STORE);
            ret = FALSE;
            goto exit;
        }
        CopyMem(image->ImageData, idata, ni->ni_Height * ni->ni_Width);
        for (x = 0; x < (ni->ni_Height * ni->ni_Width); x++) {
            idata[x] = image->Pen[image->ImageData[x]];
        }
        InitRastPort(&rp);
        rp.BitMap = image->BitMap;
        WriteChunkyPixels(&rp, 0, 0, ni->ni_Width - 1, ni->ni_Height - 1,
                          idata, ni->ni_Width);
        FreeVec(idata);

        /* Synthesize a bitmask for transparentcolor icons */
        if (image->TransparentColor >= 0) {
            int x, y;
            UBYTE *row;
            CONST UBYTE *img;
            UWORD bpr = image->BitMap->BytesPerRow;
            image->BitMask = AllocVec(bpr * ni->ni_Height + 4, MEMF_PUBLIC | MEMF_CHIP | MEMF_CLEAR);
            if (!image->BitMask) {
                SetIoErr(ERROR_NO_FREE_STORE);
                return FALSE;
            }

            img = image->ImageData;
            row = image->BitMask;
#ifdef __mc68000 /* AGA support */
            row = (APTR)(((IPTR)row + 7) & ~7);
#endif
            for (y = 0; y < ni->ni_Height; y++, row += bpr) {
                for (x = 0; x < ni->ni_Width; x++, img++) {
                    if ((*img != image->TransparentColor)) {
                        row[x>>3] |= 1 << (7 - (x & 7));
                    }
                }
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
