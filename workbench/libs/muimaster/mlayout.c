/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
