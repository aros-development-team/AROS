/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <clib/alib_protos.h>
#include <intuition/classusr.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"
#include "mui.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm VOID MUI_Redraw(register __a0 Object *obj, register __d0 ULONG flags)
#else
	AROS_LH2(VOID, MUI_Redraw,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 17, MUIMaster)
#endif
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *,MUIMasterBase)

    Object *wnd;
    Object *parent;
    struct Region *region = NULL;
    APTR clip;

    if (!(muiAreaData(obj)->mad_Flags & MADF_CANDRAW)) return;

    get(obj,MUIA_WindowObject,&wnd);
    parent = obj;

    while (get(parent,MUIA_Parent,&parent))
    {
    	if (!parent) break;
    	if (parent == wnd) break;

	if (_flags(parent) & MADF_ISVIRTUALGROUP)
	{
	    struct Rectangle rect;

	    rect.MinX = _mleft(parent);
	    rect.MinY = _mtop(parent);
	    rect.MaxX = _mright(parent);
	    rect.MaxY = _mbottom(parent);

	    if (!region)
	    {
	    	if ((region = NewRegion()))
	    	{
		    OrRectRegion(region, &rect);
	    	}
	    } else
	    {
		AndRectRegion(region, &rect);
	    }
	}
    }

    muiRenderInfo(obj)->mri_ClipRect.MinX = _left(obj);
    muiRenderInfo(obj)->mri_ClipRect.MinY = _top(obj);
    muiRenderInfo(obj)->mri_ClipRect.MaxX = _right(obj);
    muiRenderInfo(obj)->mri_ClipRect.MaxY = _bottom(obj);

    if (region)
    {
	clip = MUI_AddClipRegion(muiRenderInfo(obj),region);
    }
    DoMethod(obj, MUIM_Draw, flags);

    if (region)
    {
	/* This call actually also frees the region */
	MUI_RemoveClipRegion(muiRenderInfo(obj),region);
    }

    AROS_LIBFUNC_EXIT

} /* MUIA_Redraw */
