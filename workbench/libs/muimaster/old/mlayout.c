/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef _AROS
#include <proto/intuition.h>
#include <proto/muimaster.h>
#endif

#include <zunepriv.h>

/* Called for each children. 
 * It assigns the child to the given zone.
 */
    /*
    ** Layout function. Here, we have to call MUI_Layout() for each
    ** our children. MUI wants us to place them in a rectangle
    ** defined by (0,0,lm->lm_Layout.Width-1,lm->lm_Layout.Height-1)
    ** You are free to put the children anywhere in this rectangle.
    */
/*****************************************************************************

    NAME */
	AROS_LH6(BOOL, MUI_Layout,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj,    A0),
	AROS_LHA(LONG    , left,   D0),
	AROS_LHA(LONG    , top,    D1),
	AROS_LHA(LONG    , width,  D2),
	AROS_LHA(LONG    , height, D3),
	AROS_LHA(ULONG   , flags,  D4),

/*  LOCATION */
	struct Library *, MUIMasterBase, 21, MUIMaster)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    static const struct MUIP_Layout method = { MUIM_Layout };
    Object *parent = _parent(obj);

/*
 * Called only by groups, never by windows
 */
    ASSERT(parent != NULL);

    _left(obj) = left + _mleft(parent);
    _top(obj) = top + _mtop(parent);
    _width(obj) = width;
    _height(obj) = height;

    DoMethodA(obj, (Msg)&method);

    return TRUE;

    AROS_LIBFUNC_EXIT
}
