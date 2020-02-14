/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#define USE_BOOPSI_STUBS
#include <proto/intuition.h>
#include <proto/alib.h>
#include <clib/boopsistubs.h>
#include "datatypes_intern.h"


/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

        AROS_LH9(LONG, DrawDTObjectA,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp   , A0),
        AROS_LHA(Object          *, o    , A1),
        AROS_LHA(LONG             , x    , D0),
        AROS_LHA(LONG             , y    , D1),
        AROS_LHA(LONG             , w    , D2),
        AROS_LHA(LONG             , h    , D3),
        AROS_LHA(LONG             , th   , D4),
        AROS_LHA(LONG             , tv   , D5),
        AROS_LHA(struct TagItem  *, attrs, A2),

/*  LOCATION */
        struct Library *, DataTypesBase, 21, DataTypes)

/*  FUNCTION

    Draw a data type object into a RastPort. You must have successfully
    called ObtainDTDrawInfoA before calling this function; it invokes the
    object's DTM_DRAW method.

    INPUTS

    rp     --  pointer to the RastPort to draw the object into
    o      --  pointer to the data type object to draw
    x      --  left edge of drawing area
    y      --  top edge of drawing area
    w      --  width of drawing area
    h      --  height of drawing area
    th     --  horizontal top in units
    tv     --  vertical top in units
    attrs  --  additional attributes

    TAGS

    ADTA_Frame for animationclass objects (selects the frame that should be
    drawn.

    RESULT

    TRUE if rendering went OK, FALSE if failure.

    NOTES

    The RastPort in question must support clipping, i.e. have a valid
    layer structure attached to it; if not, some datatypes can't draw
    and FALSE will be returned.

    EXAMPLE

    BUGS

    SEE ALSO

    ObtainDataTypeA()

    INTERNALS

    The type is stated to be LONG in the original docs even if the return
    value is really BOOL.

    HISTORY

    29.7.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct dtDraw draw;

    if(rp == NULL)
        return 0;

    draw.MethodID     = DTM_DRAW;
    draw.dtd_RPort    = rp;
    draw.dtd_Left     = x;
    draw.dtd_Top      = y;
    draw.dtd_Width    = w;
    draw.dtd_Height   = h;
    draw.dtd_TopHoriz = th;
    draw.dtd_TopVert  = tv;
    draw.dtd_AttrList = attrs;

    return (LONG)DoMethodA(o, (Msg)&draw);

    AROS_LIBFUNC_EXIT
} /* DrawDTObjectA */
