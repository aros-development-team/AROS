/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    Desc: DecoratorElement - generic element rendering and compositing

    Elements are plain descriptors; how a set of them is built (from a
    theme configuration or otherwise) is the caller's business - see
    decortheme.library. The renderer holds no state of its own.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>

#include <string.h>

#include <libraries/decorator.h>
#include "decorator_intern.h"

/* ========== Element Rendering ========== */

/* Renders an element and returns the position advanced along its
   tiling axis (x for horizontal/title types, y for vertical tiling;
   non-tiling types return xp unchanged). clipw, when > 0, limits the
   x extent title tiling may draw to. */
LONG RenderElement(struct DecoratorElement *element, struct RastPort *rp,
                  ULONG subimage, LONG xp, LONG yp, LONG dw, LONG dh, LONG clipw)
{
    struct DecorImage *ni;

    if (!element || !rp)
        return xp;

    ni = element->de_Image;
    if (!ni || !ni->ok)
        return (element->de_Type == DE_TYPE_TILED_VERTICAL) ? yp : xp;

    xp += element->de_PadX;
    yp += element->de_PadY;

    switch (element->de_Type)
    {
        case DE_TYPE_STATEFUL_GADGET:
            /* Negative dimensions mean natural (unscaled) size */
            DrawScaledStatefulGadgetImageToRP(rp, ni, subimage, xp, yp, dw, dh);
            return xp;

        case DE_TYPE_TILED_HORIZONTAL:
            return WriteTiledImageHorizontal(rp, ni, subimage,
                element->de_SrcOffset, element->de_SrcSize, xp, yp, dw);

        case DE_TYPE_TILED_VERTICAL:
            return WriteTiledImageVertical(rp, ni, subimage,
                element->de_SrcOffset, element->de_SrcSize, xp, yp, dh);

        case DE_TYPE_SCALED_TILED_H:
        {
            LONG sh = element->de_SrcHeight;
            if (sh <= 0)
                sh = ni->h / (element->de_SubImageRows ? element->de_SubImageRows : 1);
            return WriteVerticalScaledTiledImageHorizontal(rp, ni, subimage,
                element->de_SrcOffset, element->de_SrcSize,
                xp, yp, sh, dw, dh);
        }

        case DE_TYPE_TILED_TITLE:
        {
            LONG rows = element->de_SubImageRows ? element->de_SubImageRows : 1;
            LONG sh = ni->h / rows;

            return WriteTiledImageTitle(TRUE, clipw, rp, ni, element->de_SrcOffset,
                subimage * sh, element->de_SrcSize, sh, xp, yp, dw, dh);
        }

        case DE_TYPE_TILED_BOTH:
            HorizVertRepeatNewImage(ni, 0x00000000, 0, 0, rp, xp, yp, dw, dh);
            return xp;

        case DE_TYPE_IMAGE:
            DrawPartImageToRP(rp, ni, xp, yp, 0, 0, ni->w, ni->h);
            return xp;

        default:
            return xp;
    }
}

/* ========== Element Compositing ========== */

/* Natural size of an element along the chain axis */
static LONG ElementNaturalSize(struct DecoratorElement *element, BOOL vertical)
{
    struct DecorImage *ni = element->de_Image;

    if (!ni)
        return 0;

    switch (element->de_Type)
    {
        case DE_TYPE_STATEFUL_GADGET:
            if (vertical)
                return ni->h / (element->de_SubImageRows ? element->de_SubImageRows : 1);
            return ni->w / (element->de_SubImageCols ? element->de_SubImageCols : 1);

        case DE_TYPE_IMAGE:
            return vertical ? ni->h : ni->w;

        default:
            /* Tiled types cover de_SrcSize pixels of source per repeat */
            return element->de_SrcSize;
    }
}

/* Renders a composite made of a chain of elements laid out along one
   axis. Elements flagged DEF_FILL share the space left over after the
   fixed size elements have been accounted for; the others render at
   their natural size. Returns the position after the last element. */
LONG RenderElementChain(struct DecoratorElement **elements, ULONG count, BOOL vertical,
                        struct RastPort *rp, ULONG subimage, LONG xp, LONG yp, LONG dw, LONG dh)
{
    LONG total = vertical ? dh : dw;
    LONG fixed = 0, fillsize = 0, fillrem = 0;
    LONG pos;
    ULONG i, nfill = 0;

    if (!elements || !rp || count == 0)
        return vertical ? yp : xp;

    for (i = 0; i < count; i++)
    {
        if (!elements[i])
            continue;
        if (elements[i]->de_Flags & DEF_FILL)
            nfill++;
        else
            fixed += ElementNaturalSize(elements[i], vertical);
    }

    if (nfill)
    {
        LONG remain = total - fixed;
        if (remain < 0) remain = 0;
        fillsize = remain / nfill;
        fillrem = remain % nfill;
    }

    pos = vertical ? yp : xp;
    for (i = 0; i < count; i++)
    {
        struct DecoratorElement *element = elements[i];
        LONG size;

        if (!element)
            continue;

        if (element->de_Flags & DEF_FILL)
        {
            size = fillsize;
            if (fillrem > 0)
            {
                size++;
                fillrem--;
            }
        }
        else
            size = ElementNaturalSize(element, vertical);

        if (size > 0)
        {
            if (vertical)
                RenderElement(element, rp, subimage, xp, pos, dw, size, 0);
            else
                RenderElement(element, rp, subimage, pos, yp, size, dh, 0);
            pos += size;
        }
    }

    return pos;
}

/* ========== Library Entry Points ========== */

AROS_LH8(LONG, DRenderElement,
    AROS_LHA(struct DecoratorElement *, element, A0),
    AROS_LHA(struct RastPort *, rp, A1),
    AROS_LHA(ULONG, subimage, D0),
    AROS_LHA(LONG, xp, D1),
    AROS_LHA(LONG, yp, D2),
    AROS_LHA(LONG, dw, D3),
    AROS_LHA(LONG, dh, D4),
    AROS_LHA(LONG, clipw, D5),
    struct Library *, DecoratorBase, 35, Decorator)
{
    AROS_LIBFUNC_INIT
    return RenderElement(element, rp, subimage, xp, yp, dw, dh, clipw);
    AROS_LIBFUNC_EXIT
}

AROS_LH9(LONG, DRenderElementChain,
    AROS_LHA(struct DecoratorElement **, elements, A0),
    AROS_LHA(ULONG, count, D0),
    AROS_LHA(BOOL, vertical, D1),
    AROS_LHA(struct RastPort *, rp, A1),
    AROS_LHA(ULONG, subimage, D2),
    AROS_LHA(LONG, xp, D3),
    AROS_LHA(LONG, yp, D4),
    AROS_LHA(LONG, dw, D5),
    AROS_LHA(LONG, dh, D6),
    struct Library *, DecoratorBase, 36, Decorator)
{
    AROS_LIBFUNC_INIT
    return RenderElementChain(elements, count, vertical, rp, subimage, xp, yp, dw, dh);
    AROS_LIBFUNC_EXIT
}
