/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>

#include "cybergraphics_intern.h"
#include "processpixelarray_ops.h"

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

        AROS_LH8(VOID, ProcessPixelArray,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp, A1),
        AROS_LHA(ULONG, destX, D0),
        AROS_LHA(ULONG, destY, D1),
        AROS_LHA(ULONG, sizeX, D2),
        AROS_LHA(ULONG, sizeY, D3),
        AROS_LHA(ULONG, operation, D4),
        AROS_LHA(LONG, value, D5),
        AROS_LHA(struct TagItem *, taglist, A2),

/*  LOCATION */
        struct Library *, CyberGfxBase, 38, Cybergraphics)

/*  FUNCTION
        Applies one of a variety of transformations to a rectangular portion
        of a RastPort, in place. The pixels are read and written as 32-bit
        ARGB, so the RastPort's bitmap should be a 15-bit or deeper
        CyberGraphX bitmap; POP_SETALPHA needs one with 4 bytes per pixel.

    INPUTS
        rp - the RastPort to process.
        destX, destY - top-lefthand corner of portion of RastPort to process.
        sizeX, sizeY - size of the affected area.
        operation - one of the following transformation types:
            POP_BLUR - blur the rectangle with a Gaussian kernel. 'value'
                (0 to 255) selects the radius, from 1 to 32 pixels. Alpha
                is blurred like the colour channels.
            POP_TINT - tint the rectangle with the ARGB32 colour in 'value':
                each colour channel is multiplied by the colour's matching
                channel / 255. Alpha is kept.
            POP_BRIGHTEN - brighten the rectangle. The amount of brightening
                to be done is defined by the 'value' input, which must be in
                the range 0 to 255.
            POP_DARKEN - darken the rectangle. The amount of darkening to be
                done is defined by the 'value' input, which must be in the
                range 0 to 255.
            POP_SETALPHA - set the alpha channel value for all pixels in the
                rectangle to that specified in the 'value' input. The valid
                range is 0 to 255.
            POP_GRADIENT, POP_COLOR2GREY, POP_NEGATIVE, POP_NEGFADE,
            POP_TINTFADE, POP_SHIFTRGB - accepted, see BUGS.
        value - see description of 'operation' input.
        taglist - parameters of POP_GRADIENT:
            PPAOPTAG_GRADIENTTYPE - GRADTYPE_HORIZONTAL or GRADTYPE_VERTICAL.
            PPAOPTAG_GRADCOLOR1 - The starting color of the gradient (ARGB32).
            PPAOPTAG_GRADCOLOR2 - The ending color of the gradient (ARGB32).
            PPAOPTAG_GRADFULLSCALE
            PPAOPTAG_GRADOFFSET

    RESULT
        None.

    NOTES

    EXAMPLE
        Frosted glass behind an application's own rendering: copy the
        background into the window's RastPort, then

            ProcessPixelArray(rp, x, y, w, h, POP_BLUR, 64, NULL);
            ProcessPixelArray(rp, x, y, w, h, POP_TINT, 0x00E0E8FF, NULL);

        and draw the foreground over it.

    BUGS
        POP_GRADIENT, POP_COLOR2GREY, POP_NEGATIVE, POP_NEGFADE,
        POP_TINTFADE and POP_SHIFTRGB are not implemented yet: the
        rectangle is left unchanged.

    SEE ALSO
        ReadPixelArray(), WritePixelArray()

    INTERNALS
        Each operation reads the rectangle a line at a time with
        ReadPixelArray(), processes it on the CPU and writes it back with
        WritePixelArray().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Rectangle    opRect;
    
    opRect.MinX = destX;
    opRect.MinY = destY;
    opRect.MaxX = opRect.MinX + sizeX - 1;
    opRect.MaxY = opRect.MinY + sizeY - 1;

    switch (operation)
    {
    case POP_BRIGHTEN:
        ProcessPixelArrayBrightnessFunc(rp, &opRect, value, CyberGfxBase);
        break;
    case POP_DARKEN:
        ProcessPixelArrayBrightnessFunc(rp, &opRect, -value, CyberGfxBase);
        break;
    case POP_SETALPHA:
        ProcessPixelArrayAlphaFunc(rp, &opRect, value, CyberGfxBase);
        break;
    case POP_TINT:
        ProcessPixelArrayTintFunc(rp, &opRect, value, CyberGfxBase);
        break;
    case POP_BLUR:
        ProcessPixelArrayBlurFunc(rp, &opRect, value, CyberGfxBase);
        break;
    case POP_COLOR2GREY:
        ProcessPixelArrayColor2GreyFunc(rp, &opRect, CyberGfxBase);
        break;
    case POP_NEGATIVE:
        ProcessPixelArrayNegativeFunc(rp, &opRect, CyberGfxBase);
        break;
    case POP_NEGFADE:
        ProcessPixelArrayNegativeFadeFunc(rp, &opRect, CyberGfxBase);
        break;
    case POP_TINTFADE:
        ProcessPixelArrayTintFadeFunc(rp, &opRect, CyberGfxBase);
        break;
    case POP_GRADIENT:
    {
        struct TagItem *tstate;
        struct TagItem *tag;
        BOOL  gradHoriz = TRUE, gradFull = FALSE;
        ULONG gradCol1 = 0, gradCol2 = 0xFFFFFF;
        ULONG gradOffset = 0;

        for (tstate = taglist; (tag = NextTagItem(&tstate)); ) {
            switch (tag->ti_Tag) {
                case PPAOPTAG_GRADIENTTYPE:
                    if (tag->ti_Data == GRADTYPE_VERTICAL)
                        gradHoriz = FALSE;
                    break;
                case PPAOPTAG_GRADCOLOR1:
                    gradCol1 = tag->ti_Data;
                    break;
                case PPAOPTAG_GRADCOLOR2:
                    gradCol2 = tag->ti_Data;
                    break;
                case PPAOPTAG_GRADFULLSCALE:
                    break;
                case PPAOPTAG_GRADOFFSET:
                    break;
                default:
                    D(bug("[Cgfx] %s: Unknown POP_GRADIENT TAG 0x%08x\n", __PRETTY_FUNCTION__, tag->ti_Tag));
                    break;
            }
        }
        ProcessPixelArrayGradientFunc(rp, &opRect, gradHoriz, gradOffset, gradCol1, gradCol2, gradFull, CyberGfxBase);
        break;
    }
    case POP_SHIFTRGB:
        ProcessPixelArrayShiftRGBFunc(rp, &opRect, CyberGfxBase);
        break;
    default:
        D(bug("[Cgfx] %s: Unhandled operation %d\n", __PRETTY_FUNCTION__, operation));
        break;
    }
    AROS_LIBFUNC_EXIT
}
