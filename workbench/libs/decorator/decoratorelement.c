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
    struct DecorImage *di;

    if (!element || !rp)
        return xp;

    di = element->de_Image;
    if (!di || !di->ok)
        return (element->de_Type == DE_TYPE_TILED_VERTICAL) ? yp : xp;

    xp += element->de_PadX;
    yp += element->de_PadY;

    switch (element->de_Type)
    {
        case DE_TYPE_STATEFUL_GADGET:
            /* Negative dimensions mean natural (unscaled) size */
            DrawScaledStatefulGadgetImageToRP(rp, di, subimage, xp, yp, dw, dh);
            return xp;

        case DE_TYPE_TILED_HORIZONTAL:
            return WriteTiledImageHorizontal(rp, di, subimage,
                element->de_SrcOffset, element->de_SrcSize, xp, yp, dw);

        case DE_TYPE_TILED_VERTICAL:
            return WriteTiledImageVertical(rp, di, subimage,
                element->de_SrcOffset, element->de_SrcSize, xp, yp, dh);

        case DE_TYPE_SCALED_TILED_H:
        {
            LONG sh = element->de_SrcHeight;
            if (sh <= 0)
                sh = di->h / (element->de_SubImageRows ? element->de_SubImageRows : 1);
            return WriteVerticalScaledTiledImageHorizontal(rp, di, subimage,
                element->de_SrcOffset, element->de_SrcSize,
                xp, yp, sh, dw, dh);
        }

        case DE_TYPE_TILED_TITLE:
        {
            LONG rows = element->de_SubImageRows ? element->de_SubImageRows : 1;
            LONG sh = di->h / rows;

            return WriteTiledImageTitle(TRUE, clipw, rp, di, element->de_SrcOffset,
                subimage * sh, element->de_SrcSize, sh, xp, yp, dw, dh);
        }

        case DE_TYPE_TILED_BOTH:
            HorizVertRepeatDecorImage(di, 0x00000000, 0, 0, rp, xp, yp, dw, dh);
            return xp;

        case DE_TYPE_IMAGE:
            DrawPartImageToRP(rp, di, xp, yp, 0, 0, di->w, di->h);
            return xp;

        default:
            return xp;
    }
}

/* ========== Element Compositing ========== */

/* Natural size of an element along the chain axis */
static LONG ElementNaturalSize(struct DecoratorElement *element, BOOL vertical)
{
    struct DecorImage *di = element->de_Image;

    if (!di)
        return 0;

    switch (element->de_Type)
    {
        case DE_TYPE_STATEFUL_GADGET:
            if (vertical)
                return di->h / (element->de_SubImageRows ? element->de_SubImageRows : 1);
            return di->w / (element->de_SubImageCols ? element->de_SubImageCols : 1);

        case DE_TYPE_IMAGE:
            return vertical ? di->h : di->w;

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

/*****************************************************************************
    NAME
        LONG DRenderElement(

    SYNOPSIS
        struct DecoratorElement *element, struct RastPort *rp, ULONG subimage, LONG xp, LONG yp, LONG dw, LONG dh, LONG clipw)

    FUNCTION
        Draw an image element using its type, source region and padding.

    INPUTS
        element - Element descriptor, or NULL to do nothing.
        rp - Destination RastPort, or NULL to do nothing.
        subimage - Gadget state for DE_TYPE_STATEFUL_GADGET; grid row for
            horizontal tiling, column for vertical tiling, or source row
            selector for title tiling. Ignored for plain and repeated images.
        xp, yp - Destination origin before de_PadX and de_PadY are added.
        dw, dh - Requested extents. Meaning and valid ranges depend on type.
        clipw - Positive absolute right limit for title tiling; otherwise ignored.

    RESULT
        Final y for vertical tiling, final x for horizontal/title tiling,
        or padded xp for other types. A missing or invalid image returns
        unpadded yp for vertical tiling, otherwise unpadded xp. NULL element
        or rp returns unpadded xp regardless of type. Not a drawing status.

    NOTES
        STATEFUL_GADGET scales the selected state to dw by dh; negative
        sizes use natural dimensions. TILED_HORIZONTAL and TILED_VERTICAL
        repeat de_SrcSize pixels starting at de_SrcOffset on their axis.
        SCALED_TILED_H also scales to dh, using de_SrcHeight when positive
        or image height divided by de_SubImageRows (zero means one row).
        TILED_TITLE derives sy from subimage and de_SubImageRows, tiles to dw
        and scales to dh. TILED_BOTH repeats the full image over dw by dh.
        IMAGE copies the full image and ignores dw and dh. Unknown types
        produce no output. de_Flags does not affect this function.

        Padding moves the origin without shrinking the extents. Descriptor
        row and column counts do not replace the image's own grid; keep them
        consistent. Source rectangles, positive tile sizes and storage must
        satisfy the corresponding drawing functions. State and repeated-image
        sizes narrow to WORD, and some coordinates narrow to UWORD; keep
        values representable. Inputs are borrowed and not modified.

    BUGS
        Title tiling does not honor its derived source height, so row
        selection can read past the intended cell. Scaled types inherit
        DScaleBuffer() allocation failures. No error status is available.

    SEE ALSO
        DRenderElementChain(), DWriteTiledImageTitle(), DWriteTiledImageHorizontal(),
        DWriteTiledImageVertical(), DWriteVerticalScaledTiledImageHorizontal(),
        DDrawScaledStatefulGadgetImageToRP(), DHorizVertRepeatDecorImage(), DDrawPartImageToRP()

*****************************************************************************/
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

/*****************************************************************************
    NAME
        LONG DRenderElementChain(

    SYNOPSIS
        struct DecoratorElement **elements, ULONG count, BOOL vertical, struct RastPort *rp, ULONG subimage, LONG xp, LONG yp, LONG dw, LONG dh)

    FUNCTION
        Lay out and draw an ordered chain of elements along one axis.

    INPUTS
        elements - Array of count descriptor pointers. NULL entries are skipped.
        count - Number of pointers in the array.
        vertical - TRUE to advance y; FALSE to advance x.
        rp - Destination RastPort with a writable bitmap.
        subimage - State or subimage selector passed to each element.
        xp, yp - Initial destination origin, in pixels.
        dw, dh - Available extents; dh is the chain extent when vertical,
            otherwise dw. Provide nonnegative dimensions.

    RESULT
        Position after the positive sizes assigned to elements, on the chain
        axis. NULL elements, NULL rp or zero count returns the starting axis
        position. The result describes layout, not successful drawing.

    NOTES
        Fixed elements use natural size on the chain axis: the full image
        size for IMAGE, image size divided by the descriptor grid count for
        STATEFUL_GADGET, and de_SrcSize for other types. A zero descriptor
        grid count means one. A missing image contributes zero fixed size.

        DEF_FILL elements share max(available - fixed, 0) equally; the first
        such elements receive any remainder pixels. Other flags are ignored.
        Entries assigned nonpositive sizes are not drawn or advanced. Fixed
        sizes are not clipped to the available extent and can exceed it.

        Each call to DRenderElement() receives clipw == 0. Padding changes
        drawing positions but does not change the layout advance. An invalid
        image can still occupy space; drawing return values are ignored.
        There is no chain-level clipping or guarantee that an element draws
        within its assigned size. Keep descriptors, source images and buffers
        valid for the call, with nonnegative natural sizes and no arithmetic
        overflow. All drawing preconditions of DRenderElement() still apply.
        Inputs are borrowed; only the destination drawing state is changed.

    SEE ALSO
        DRenderElement()

*****************************************************************************/
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
