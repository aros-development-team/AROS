/*
    Copyright � 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <clib/alib_protos.h>
#include <intuition/classusr.h>
#include <graphics/gfxmacros.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/cybergraphics.h>

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

/*****************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_Redraw,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 17, MUIMaster)

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
        if (_parent(obj))
        {
            ULONG parentDisabled;
            if (get(_parent(obj), MUIA_Disabled, &parentDisabled))
            {
                /* Let the parent draw the pattern... */
                if (parentDisabled) disabled = FALSE;
            }
        }
        
	if (disabled)
	{
#if 0
            /*
                This aproach might be faster *provided* that the buffer is
                allocated and filled *once* at startup of muimaster.library.
                
                In reality, the WritePixelArray() call has quite a big 
                overhead, so you should only use this buffer if the gadget
                completely fits inside, and fall back to allocating a new
                buffer if the gadget is too big.
                
                Perhaps a future optimization...
            */
            LONG  width  = 200;
            LONG  height = 100;
            LONG *buffer = AllocVec(width * height * sizeof(LONG), MEMF_ANY);
            LONG  x, y;
            
            memset(buffer, 0xAA, width * height * sizeof(LONG));
            
            for (y = 0; y < _height(obj); y += height)
            {
                for (x = 0; x < _width(obj); x += width)
                {
                    WritePixelArrayAlpha
                    (
                        buffer, 0, 0, width * sizeof(LONG),
                        _rp(obj), _left(obj) + x, _top(obj) + y, 
                        x + width  > _width(obj)  ? _width(obj)  - x : width,
                        y + height > _height(obj) ? _height(obj) - y : height,
                        0
                    );
                }
            }

#else
            
            LONG  width  = _width(obj);
            LONG  height = _height(obj);
            LONG *buffer = AllocVec(width * height * sizeof(LONG), MEMF_ANY);

            if (buffer != NULL)
            {
#if 1
                memset(buffer, 0xAA, width * height * sizeof(LONG));
#else
                LONG color = (0xAA << 24) + (0xAA << 16) + (0xAA << 8) + 0xBB;
                LONG x, y;
                
                for (y = 0; y < height; y++)
                {
                    for (x = 0; x < width; x++)
                    {
                        buffer[y * width + x] = color;
                    }
                }
#endif
                
                WritePixelArrayAlpha
                (
                    buffer, 0, 0, width * sizeof(LONG), 
                    _rp(obj), _left(obj), _top(obj), width, height, 0
                );
                
                FreeVec(buffer);
            }
#endif
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
