/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/alib_protos.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "textengine.h"
#include "bubbleengine.h"
#include "support.h"

#include "muimaster_intern.h"

//#define MYDEBUG 1
#include "debug.h"

extern struct Library *MUIMasterBase;

/************************************************************************/

#ifndef __AROS__
#define WA_Shape WA_ShapeRegion
#endif

/************************************************************************/

typedef struct ZBubble
{
    struct Window *win;
    ZText   	  *ztext;
    STRPTR  	   text;
    UBYTE   	   flags;
    struct Region *shape;
} ZBubble;

#define BUBBLEF_CREATESHORTHELP_CALLED 1

/************************************************************************/


/*                [][][][]
              [][]
          [][]
        []
      []
    []
    []
  []
  []
[]
[]
[]
[]

*/

#define ROUNDEDGE_SIZE 13
#define BORDER_X 5
#define BORDER_Y 5

static WORD roundtab[ROUNDEDGE_SIZE] =
{
    9,
    7,
    5,
    4,
    3,
    2,
    2,
    1,
    1,
    0,
    0,
    0,
    0
};

static struct Region *zune_bubble_shape_create(Object *obj, ZBubble *bubble, WORD w, WORD h)
{
    struct Region   	*shape = NewRegion();
    struct Rectangle 	 r;
    WORD    	    	 y;
    
    if (!shape) return NULL;
    
    for(y = 0; y < ROUNDEDGE_SIZE; y++)
    {	
    	r.MinX = roundtab[y];
	r.MinY = y;
	r.MaxX = w - 1 - r.MinX;
	r.MaxY = y;
	
	OrRectRegion(shape, &r);
	
	r.MinY = h - 1 - y;
	r.MaxY = h - 1 - y;
	
	OrRectRegion(shape, &r);	
    }
    
    r.MinX = 0;
    r.MinY = y;
    r.MaxX = w - 1;
    r.MaxY = h - 1 - y;
    
    OrRectRegion(shape, &r);
    
    return shape;

}

static void zune_bubble_draw(Object *obj, ZBubble *bubble)
{
    struct RastPort *rp = bubble->win->RPort, *objrp;
    WORD    	     x1, x2, y, w, h, prev_x1 = -1;
    
    w = bubble->win->Width;
    h = bubble->win->Height;
    
    for(y = 0; y < ROUNDEDGE_SIZE; y++, prev_x1 = x1)
    {
    	x1 = roundtab[y];
	x2 = w - 1 - x1;
	
	if (prev_x1 == -1)
	{
    	    SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
	}
	else
	{
    	    SetAPen(rp, _pens(obj)[MPEN_SHINE]);
	}
	
	RectFill(rp, x1, y, x2, y);
	RectFill(rp, x1, h - 1 - y, x2, h - 1 - y);
	
	if (prev_x1 != -1)
	{
	    if (prev_x1 != x1) prev_x1--;
    	    SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
	    RectFill(rp, x1, y, prev_x1, y);
	    RectFill(rp, x1, h - 1 - y, prev_x1, h - 1 - y);
	    RectFill(rp, w - 1 - prev_x1, y, w - 1 - x1, y);
	    RectFill(rp, w - 1 - prev_x1, h - 1 - y, w - 1 - x1, h - 1 - y);    
	}

    }
    
    SetAPen(rp, _pens(obj)[MPEN_SHINE]);
    RectFill(rp, 1, y, w - 2, h - 1 - y);
    
    SetAPen(rp, _pens(obj)[MPEN_SHADOW]);    
    RectFill(rp, 0, y, 0, h - 1 - y);
    RectFill(rp, w - 1, y, w - 1, h - 1 - y);
    
    objrp = _rp(obj);
    _rp(obj) = rp;
    
    zune_text_draw(bubble->ztext, obj, ROUNDEDGE_SIZE + BORDER_X, 
    	    	   ROUNDEDGE_SIZE + BORDER_X + bubble->ztext->width - 1,
		   (bubble->win->Height - bubble->ztext->height + 1) / 2);
		   
   _rp(obj) = objrp;
}

APTR zune_bubble_create(Object *obj, LONG x, LONG y, char *text, ULONG flags)
{
    ZBubble *bubble;
    
    if (!(_flags(obj) & MADF_CANDRAW)) return NULL;
    
    bubble = mui_alloc_struct(ZBubble);
    if (!bubble) return NULL;
    
    if (!text)
    {
    	text = (STRPTR)DoMethod(obj, MUIM_CreateShortHelp, x, y);
	bubble->flags |= BUBBLEF_CREATESHORTHELP_CALLED;	
    }
    
    if (!text)
    {
    	mui_free(bubble);	
	return NULL;
    }
    
    bubble->text = text;

    bubble->ztext = zune_text_new(NULL, bubble->text, ZTEXT_ARG_NONE, 0);
    if (!bubble->ztext)
    {
    	zune_bubble_delete(obj, bubble);	
	return NULL;
    }
    
    zune_text_get_bounds(bubble->ztext, obj);
    
    #define WINLEFT   _window(obj)->LeftEdge + x
    #define WINTOP    _window(obj)->TopEdge  + y
    
    #define WINWIDTH  bubble->ztext->width   + (ROUNDEDGE_SIZE + BORDER_X) * 2
    #define WINHEIGHT ((bubble->ztext->height  + BORDER_Y * 2) < ROUNDEDGE_SIZE * 2 + 1) ? \
    	    	      ROUNDEDGE_SIZE * 2 + 1 : (bubble->ztext->height + BORDER_Y * 2)
    
    bubble->shape = zune_bubble_shape_create(obj, bubble, WINWIDTH, WINHEIGHT);

    bubble->win = OpenWindowTags(NULL, WA_CustomScreen, (IPTR)_screen(obj),
    	    	    	    	       WA_Left, WINLEFT,
				       WA_Top, WINTOP,
				       WA_Width, WINWIDTH,
				       WA_Height, WINHEIGHT,
				       WA_AutoAdjust, TRUE,
				       WA_Activate, FALSE,
				       WA_Borderless, TRUE,
				       WA_BackFill, (IPTR)LAYERS_NOBACKFILL,
				       WA_Shape, (IPTR)bubble->shape,
				       TAG_DONE);

    if (!bubble->win)
    {
    	zune_bubble_delete(obj, bubble);
	return NULL;
    }
    
    zune_bubble_draw(obj, bubble);

    return bubble;
}

void zune_bubble_delete(Object *obj, APTR bubble)
{
    ZBubble *b = (ZBubble *)bubble;
    
    if (b)
    {
    	if (b->win) CloseWindow(b->win);
	if (b->shape) DisposeRegion(b->shape);
    	if (b->ztext) zune_text_destroy(b->ztext);
	
    	if (b->flags & BUBBLEF_CREATESHORTHELP_CALLED)
	{
	    DoMethod(obj, MUIM_DeleteShortHelp, (IPTR)b->text);
	}
	
    	mui_free(b);
    }
}

