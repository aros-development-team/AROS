/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <intuition/classusr.h>
#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"

//#define MYDEBUG 1
#include "debug.h"

/*****************************************************************************

    NAME */
        AROS_LH6(BOOL, MUI_Layout,

/*  SYNOPSIS */
        AROS_LHA(Object *, obj, A0),
        AROS_LHA(LONG, left, D0),
        AROS_LHA(LONG, top, D1),
        AROS_LHA(LONG, width, D2),
        AROS_LHA(LONG, height, D3),
        AROS_LHA(ULONG, flags, D4),

/*  LOCATION */
        struct Library *, MUIMasterBase, 21, MUIMaster)

/*  FUNCTION
        Set a child object's geometry relative to its parent's inner origin,
        then invoke MUIM_Layout on the child. For a virtual-group parent,
        subtract the parent's horizontal and vertical scroll offsets first.

    INPUTS
        obj - Non-NULL Area-derived child with a valid parent and initialized
              layout data. This helper is used by group layout code.
        left, top - Position relative to the parent's inner origin before
                    virtual-group scroll offsets are subtracted.
        width, height - Dimensions stored in the child's geometry.
        flags - Ignored by this implementation; pass zero.

    RESULT
        Always TRUE after the layout method returns. The result of
        MUIM_Layout is discarded.

    NOTES
        This helper neither validates the dimensions against the child's
        minimum and maximum sizes nor requests a redraw. The layout owner
        must supply suitable geometry. NULL objects and parentless objects
        are not accepted.

    SEE ALSO
        MUI_Redraw()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    static const struct MUIP_Layout method = { MUIM_Layout };
    Object *parent = _parent(obj);

/*
 * Called only by groups, never by windows
 */
//    ASSERT(parent != NULL);

    if (_flags(parent) & MADF_ISVIRTUALGROUP)
    {
        /* I'm not yet sure what to do by virtual groups in virtual groups,
         * eighter add their offsets too or not, will be tested soon */
        IPTR val = 0;
        get(parent,MUIA_Virtgroup_Left,&val);
        left -= val;
        get(parent,MUIA_Virtgroup_Top,&val);
        top -= val;
    }

    _left(obj) = left + _mleft(parent);
    _top(obj) = top + _mtop(parent);
    _width(obj) = width;
    _height(obj) = height;

    D(bug("muimaster.library/mui_layout.c: 0x%p %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj)));

    DoMethodA(obj, (Msg)&method);
    return TRUE;

    AROS_LIBFUNC_EXIT

} /* MUIA_Layout */
