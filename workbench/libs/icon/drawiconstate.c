/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <workbench/icon.h>
#include <intuition/imageclass.h>

#include "icon_intern.h"

/* Define this in order to simulate LUT screen on hi- and truecolor displays.
   Useful for debugging.
#define FORCE_LUT_ICONS */

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH7(void, DrawIconStateA,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *,   rp,       A0),
	AROS_LHA(struct DiskObject *, icon,     A1),
	AROS_LHA(STRPTR,              label,    A2),
	AROS_LHA(LONG,                leftEdge, D0),
	AROS_LHA(LONG,                topEdge,  D1),
	AROS_LHA(ULONG,               state,    D2),
	AROS_LHA(struct TagItem *,    tags,     A3),

/*  LOCATION */
	struct IconBase *, IconBase, 27, Icon)

/*  FUNCTION
	Draw an icon like an image.
	
    INPUTS
	rp       - rastport to draw into
	icon     - the icon
	label    - label string
	leftEdge, 
	topEdge  - drawing position 
	state    - drawing state, see intuition/imageclass.h
    
    RESULT

    NOTES
	Only very limited implemented.

    EXAMPLE

    BUGS

    SEE ALSO
	intuition/imageclass.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG wDelta,textTop,textLeft;
    struct NativeIcon *ni;
    struct Rectangle rect = { };
    ULONG bmdepth;
    struct DrawInfo *DrawInfo;
    BOOL EraseBackground;
    BOOL Frameless;
    BOOL Borderless;
    BOOL selected = (state == IDS_SELECTED);
    UWORD Pens[NUMDRIPENS];
    int i;

    /* No GfxBase? No RastPort? Then we can't draw anything. */
    if (!GfxBase || !rp)
        return;

    ni = GetNativeIcon(icon, LB(IconBase));

    Frameless = GetTagData(ICONDRAWA_Frameless, IconBase->ib_Frameless || ni->ni_Frameless, tags);
    Borderless = GetTagData(ICONDRAWA_Borderless, FALSE, tags);
    EraseBackground = GetTagData(ICONDRAWA_EraseBackground, !(IconBase->ib_Frameless || ni->ni_Frameless), tags);
    DrawInfo = (struct DrawInfo *)GetTagData(ICONDRAWA_DrawInfo, (IPTR)NULL, tags);

    for (i = 0; i < NUMDRIPENS; i++) {
        if (DrawInfo && DrawInfo->dri_NumPens > i)
            Pens[i] = DrawInfo->dri_Pens[i];
        else
            Pens[i] = ni->ni_Pens[i];
    }

    bmdepth = GetBitMapAttr(rp->BitMap, BMA_DEPTH);

    if (!Borderless) {
        rect = IconBase->ib_EmbossRectangle;
        if (!Frameless) {
            EraseBackground = TRUE;
            rect.MinX -= 1;
            rect.MinY -= 1;
            rect.MaxX += 1;
            rect.MaxY += 1;
        }
    } else {
        Frameless = TRUE;
    }

    wDelta = 0;
    textTop = 0;
    textLeft = 0;

    if (label != NULL) {
        struct TextExtent extent;
        LONG txtlen = strlen(label);

        if (txtlen > IconBase->ib_MaxNameLength)
            txtlen = IconBase->ib_MaxNameLength;

        wDelta = ni->ni_Width + (rect.MaxX - rect.MinX);
        textTop = ni->ni_Height + (rect.MaxY - rect.MinY);

        TextExtent(rp, label, txtlen, &extent);

        /* wDelta will be the centering offset for the icon */
        /* textLeft is the horiz offset for the text */

        if (extent.te_Width > wDelta) {
            wDelta = (extent.te_Width - wDelta) / 2;
            textLeft = 0;
        } else {
            textLeft = (wDelta - extent.te_Width) / 2;
            wDelta = 0;
        }

        /* textTop is adjusted downwards by the size of the font,
         */
        textTop += extent.te_Height;

        SetAPen(rp, Pens[FILLTEXTPEN]);
        SetOutlinePen(rp, selected ? Pens[HIGHLIGHTTEXTPEN] : Pens[TEXTPEN]);
        SetBPen(rp, Pens[BACKGROUNDPEN]);

        Move(rp, leftEdge + textLeft, topEdge + textTop);
        Text(rp, label, txtlen);
    }
    
    topEdge -= rect.MinY;
    leftEdge -=  rect.MinX;

    leftEdge += wDelta;

    if (EraseBackground) {
        SetAPen(rp, Pens[BACKGROUNDPEN]);
        RectFill(rp, leftEdge + rect.MinX, topEdge + rect.MinY,
                     leftEdge + ni->ni_Width + rect.MaxX - 1, topEdge + ni->ni_Height + rect.MaxY - 1);
    }

    D(bug("[%s] Target bitmap depth: %d\n", __func__, bmdepth));

    if (ni)
    {
        struct NativeIconImage *image;
        int id;
        struct BitMap *bm;
        PLANEPTR mask;

        id = selected ? 1 : 0;
        image = &ni->ni_Image[id];
           
#ifndef FORCE_LUT_ICONS
        if ((bmdepth > 8) && CyberGfxBase)
	{
	    if (ni->ni_Extra.PNG[id].Size && image->ARGB == NULL) {
                image->ARGB = ReadMemPNG(icon, ni->ni_Extra.Data + ni->ni_Extra.PNG[id].Offset, &ni->ni_Width, &ni->ni_Height, NULL, NULL, IconBase);
            }
            if (image->ARGB) {
                D(bug("[%s] ARGB[%d] = %p\n", __func__, id, image->ARGB));

                WritePixelArrayAlpha((APTR)image->ARGB, 0, 0, ni->ni_Width * sizeof(ULONG),
                                     rp, leftEdge, topEdge,
                                     ni->ni_Width,  ni->ni_Height, 0);
                goto emboss;
            }
	}
#endif

        /* If we don't have selected bitmap,
         * use the normal bitmap
         */
        bm = image->BitMap;
        mask = image->BitMask;
        if (bm == NULL) {
            bm = ni->ni_Image[0].BitMap;
            mask = ni->ni_Image[0].BitMask;
        }
#ifdef __mc68000 /* AGA support */
        /* Get the 64 bit aligned mask */
        mask = (APTR)(((IPTR)mask + 7) & ~7);
#endif

        /* Planar maps */
	if (bm)
	{
#if DEBUG
	    int i;

	    bug("[%s] Using Bitmap: 0x%p, BitMask 0x%p\n", __func__, bm, mask);
	    for (i = 0; i < bm->Depth; i++)
	        bug("[%s] Planes[%d] = %p\n", __func__, i, bm->Planes[i]);
#endif

	    if (mask) {
                BltMaskBitMapRastPort(bm, 0, 0,
                                      rp, leftEdge, topEdge,
                                      ni->ni_Width, ni->ni_Height, ABC|ABNC|ANBC, mask);
            } else {
                BltBitMapRastPort(bm, 0, 0,
                                  rp, leftEdge, topEdge,
                                  ni->ni_Width, ni->ni_Height, ABC|ABNC);
            }
            goto emboss;
        }
    }

    /* Uh. No layout? Just draw a square.
     */
    D(bug("[Icon] No image present\n"));
    SetAPen(rp, selected ? Pens[SHINEPEN] : Pens[SHADOWPEN]);
    RectFill(rp, leftEdge, topEdge,
                 leftEdge + ni->ni_Width - 1,
                 topEdge + ni->ni_Height - 1);

emboss:
    /* Draw the 3D border */
    if (!Frameless) {
        D(bug("[Icon] Embossing\n"));

        rect.MinX += leftEdge;
        rect.MaxX += leftEdge + ni->ni_Width - 1;
        rect.MinY += topEdge;
        rect.MaxY += topEdge + ni->ni_Height - 1;

        /* Draw the left and top lines */
        SetAPen(rp, selected ? Pens[SHADOWPEN] : Pens[SHINEPEN]);
        Move(rp, rect.MinX, rect.MaxY - 1);
        Draw(rp, rect.MinX, rect.MinY);
        Draw(rp, rect.MaxX - 1, rect.MinY);

        /* Draw the right and bottom lines */
        SetAPen(rp, selected ? Pens[SHINEPEN] : Pens[SHADOWPEN]);
        Move(rp, rect.MaxX, rect.MinY + 1);
        Draw(rp, rect.MaxX, rect.MaxY);
        Draw(rp, rect.MinX + 1, rect.MaxY);

        SetAPen(rp, Pens[BACKGROUNDPEN]);
        WritePixel(rp, rect.MinX, rect.MaxY);
        WritePixel(rp, rect.MaxX, rect.MinY);

    }

    AROS_LIBFUNC_EXIT
} /* DrawIconStateA() */
