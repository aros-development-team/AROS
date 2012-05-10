/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
    struct DrawInfo *DrawInfo;
    BOOL EraseBackground;
    BOOL Frameless;
    BOOL Borderless;
    BOOL selected = (state == IDS_SELECTED);
    UWORD Pens[NUMDRIPENS];
    UWORD Width, Height;
    int i;

    /* No GfxBase? No RastPort? Then we can't draw anything. */
    if (!GfxBase || !rp)
        return;

    ni = GetNativeIcon(icon, LB(IconBase));

    Frameless = GetTagData(ICONDRAWA_Frameless, IconBase->ib_Frameless || (ni && ni->ni_Frameless), tags);
    Borderless = GetTagData(ICONDRAWA_Borderless, FALSE, tags);
    EraseBackground = GetTagData(ICONDRAWA_EraseBackground, !(IconBase->ib_Frameless || (ni && ni->ni_Frameless)), tags);
    DrawInfo = (struct DrawInfo *)GetTagData(ICONDRAWA_DrawInfo, (IPTR)NULL, tags);

    for (i = 0; i < NUMDRIPENS; i++) {
        if (DrawInfo && DrawInfo->dri_NumPens > i)
            Pens[i] = DrawInfo->dri_Pens[i];
        else
            Pens[i] = ni->ni_Pens[i];
    }

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

    if (ni && ni->ni_Screen) {
        Width = ni->ni_Width;
        Height = ni->ni_Height;
    } else {
        Width = icon->do_Gadget.Width;
        Height = icon->do_Gadget.Height;
    }

    if (label != NULL) {
        struct TextExtent extent;
        LONG txtlen = strlen(label);

        if (txtlen > IconBase->ib_MaxNameLength)
            txtlen = IconBase->ib_MaxNameLength;

        wDelta = Width + (rect.MaxX - rect.MinX);
        textTop = Height + (rect.MaxY - rect.MinY);

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

        SetAPen(rp, Pens[FILLTEXTPEN]);
        SetOutlinePen(rp, selected ? Pens[HIGHLIGHTTEXTPEN] : Pens[TEXTPEN]);
        SetBPen(rp, Pens[BACKGROUNDPEN]);

        Move(rp, leftEdge + textLeft, topEdge + textTop - extent.te_Extent.MinY);
        Text(rp, label, txtlen);
    }
    
    topEdge -= rect.MinY;
    leftEdge -=  rect.MinX;

    leftEdge += wDelta;

    if (EraseBackground) {
        SetAPen(rp, Pens[BACKGROUNDPEN]);
        RectFill(rp, leftEdge + rect.MinX, topEdge + rect.MinY,
                     leftEdge + Width + rect.MaxX - 1, topEdge + Height + rect.MaxY - 1);
    }

    D(bug("[%s] Render %ldx%ld icon %p at %ldx%ld\n", __func__, Width, Height, icon, leftEdge, topEdge));

    if (ni)
    {
        struct NativeIconImage *image;
        int id;
        struct BitMap *bm;
        PLANEPTR mask;

        id = selected ? 1 : 0;
        image = &ni->ni_Image[id];
         
        if (image->ARGBMap) {
            WritePixelArrayAlpha(image->ARGBMap, 0, 0, ni->ni_Width * sizeof(ULONG),
                                       rp, leftEdge, topEdge,
                                       ni->ni_Width,  ni->ni_Height, 0);
            goto emboss;
        }

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
                                      Width, Height, ABC|ABNC|ANBC, mask);
            } else {
                BltBitMapRastPort(bm, 0, 0,
                                  rp, leftEdge, topEdge,
                                  Width, Height, ABC|ABNC);
            }
            goto emboss;
        }
    }

    /* Uh. No layout? Just draw a square.
     */
    D(bug("[Icon] No image present\n"));
    SetAPen(rp, selected ? Pens[SHINEPEN] : Pens[SHADOWPEN]);
    RectFill(rp, leftEdge, topEdge,
                 leftEdge + Width - 1,
                 topEdge + Height - 1);

emboss:
    /* Draw the 3D border */
    if (!Frameless) {
        D(bug("[Icon] Embossing\n"));

        rect.MinX += leftEdge;
        rect.MaxX += leftEdge + Width - 1;
        rect.MinY += topEdge;
        rect.MaxY += topEdge + Height - 1;

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
