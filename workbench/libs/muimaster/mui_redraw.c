/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <clib/alib_protos.h>
#include <intuition/classusr.h>
#include <graphics/gfxmacros.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

/*****************************************************************************

    NAME */
#ifndef __AROS__
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
    struct Rectangle *clip_rect;
    ULONG disabled;

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

    clip_rect = &muiRenderInfo(obj)->mri_ClipRect;

    if (region)
    {
    	/* Maybe this should went to MUI_AddClipRegion() */
    	clip_rect->MinX = MAX(_left(obj),region->bounds.MinX);
    	clip_rect->MinY = MAX(_top(obj),region->bounds.MinY);
    	clip_rect->MaxX = MIN(_right(obj),region->bounds.MaxX);
    	clip_rect->MaxY = MIN(_bottom(obj),region->bounds.MaxY);

	clip = MUI_AddClipRegion(muiRenderInfo(obj),region);
    } else
    {
        clip_rect->MinX = _left(obj);
        clip_rect->MinY = _top(obj);
        clip_rect->MaxX = _right(obj);
        clip_rect->MaxY = _bottom(obj);
    }
    DoMethod(obj, MUIM_Draw, flags);

    if (get(obj, MUIA_Disabled, &disabled))
    {
	if (disabled)
	{
	    const static UWORD pattern[] = {
		0x5555,
		0xaaaa,
	    };
	    LONG fg = muiRenderInfo(obj)->mri_Pens[MPEN_SHADOW];
	    SetDrMd(_rp(obj), JAM1);
	    SetAPen(_rp(obj), fg);
	    SetAfPt(_rp(obj), pattern, 1);
	    RectFill(_rp(obj), _left(obj), _top(obj), _right(obj), _bottom(obj));
	    SetAfPt(_rp(obj), NULL, 0);
	}
    }

    /* copy buffer to window */
    if (muiRenderInfo(obj)->mri_BufferBM)
    {
	ClipBlit(&muiRenderInfo(obj)->mri_BufferRP, _left(obj), _top(obj),
		 muiRenderInfo(obj)->mri_Window->RPort, _left(obj), _top(obj),
		 _width(obj), _height(obj), 0xc0);
    }

    if (region)
    {
	/* This call actually also frees the region */
	MUI_RemoveClipRegion(muiRenderInfo(obj),region);
    }

    AROS_LIBFUNC_EXIT

} /* MUIA_Redraw */
