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

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

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

    /* TODO: Implement me */

    bug("ProcessPixelArray not implemented\n");

#if 0
    struct TagItem *tstate;
    struct TagItem *tag;

    for (tstate = tags; (tag = NextTagItem(&tstate)); ) {
	switch (tag->ti_Tag) {
	    case aaa:
	    	minwidth = (ULONG)tag->ti_Data;
		break;
		

	    default:
	    	D(bug("!!! UNKNOWN TAG PASSED TO ProcessPixelArray\n"));
		break;
	} 	
    }

#endif

    AROS_LIBFUNC_EXIT
}
