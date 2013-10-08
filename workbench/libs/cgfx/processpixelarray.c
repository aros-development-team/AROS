/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
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
        of a RastPort.

    INPUTS
        rp - the RastPort to process.
        destX, destY - top-lefthand corner of portion of RastPort to process.
        sizeX, sizeY - size of the affected area.
        operation - one of the following transformation types:
            POP_TINT - tint the rectangle with an ARGB32 color ('value' input).
            POP_BLUR - blur the rectangle.
            POP_BRIGHTEN - brighten the rectangle. The amount of brightening
                to be done is defined by the 'value' input, which must be in
                the range 0 to 255.
            POP_DARKEN - darken the rectangle. The amount of darkening to be
                done is defined by the 'value' input, which must be in the
                range 0 to 255.
            POP_SETALPHA - set the alpha channel value for all pixels in the
                rectangle to that specified in the 'value' input. The valid
                range is 0 to 255.
            POP_GRADIENT - apply a gradient to the rectangle. Gradient
                parameters are supplied through the taglist.
        value - see description of 'operation' input.
        taglist - currently describes gradient parameters, as follows:
            PPAOPTAG_GRADIENTTYPE - GRADTYPE_HORIZONTAL or GRADTYPE_VERTICAL.
            PPAOPTAG_GRADCOLOR1 - The starting color of the gradient (ARGB32).
            PPAOPTAG_GRADCOLOR2 - The ending color of the gradient (ARGB32).
            PPAOPTAG_GRADFULLSCALE
            PPAOPTAG_GRADOFFSET

    RESULT
        count - the number of pixels processed.

    NOTES

    EXAMPLE

    BUGS
        This function is not implemented.

    SEE ALSO

    INTERNALS
        This function exists to get Scalos compiled. Because Scalos
        has its own fallback code for the case that lib_Version < 50
        it's not so urgent to implement it.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Rectangle    opRect;
    
    opRect.MinX = destX;
    opRect.MaxX = destY;
    opRect.MinY = opRect.MinX + sizeX - 1;
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
        ProcessPixelArrayBlurFunc(rp, &opRect, CyberGfxBase);
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
