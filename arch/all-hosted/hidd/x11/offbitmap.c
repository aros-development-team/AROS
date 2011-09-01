/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Offscreen bitmap class for X11 hidd.
    Lang: English.
*/

/****************************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <string.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>
#include <hidd/graphics.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "x11gfx_intern.h"
#include "x11.h"
#include "bitmap.h"

/****************************************************************************************/

BOOL X11BM_InitPM(OOP_Class *cl, OOP_Object *o, struct TagItem *attrList)
{
    OOP_Object *friend;
    Drawable friend_drawable = 0;
    IPTR depth;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    /* Retrieve bitmap size from superclass */
    OOP_GetAttr(o, aHidd_BitMap_Width , &data->width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &data->height);
    OOP_GetAttr(o, aHidd_BitMap_Depth , &depth);

    friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, attrList);
    if (friend)
    {
    	/* Get the X11 window from the friend bitmap */
	OOP_GetAttr(friend, aHidd_X11BitMap_Drawable, &friend_drawable);
    }
    
    if (!friend_drawable)
    {
	/* If no friend, or friend is not X11 bitmap, use default friend drawable */
	friend_drawable = XSD(cl)->dummy_window_for_creating_pixmaps;
    }

    /* 
     * We must only create depths that are supported by the friend drawable
     * Currently we only support the default depth, and depth 1
     */
    if (depth != 1)
    {
	depth = DefaultDepth(data->display, data->screen);
    }
    else
    {
    	/* Need this because of stipple bug in XFree86 :-( */
	data->width += 32;
    }

    D(bug("Creating X Pixmap, 0x%p, %ld, %ld, %ld\n", friend_drawable, data->width, data->height, depth));

    HostLib_Lock();

    DRAWABLE(data) = XCALL(XCreatePixmap, data->display, friend_drawable, data->width, data->height, depth);
    XCALL(XFlush, data->display);

    HostLib_Unlock();

    return DRAWABLE(data) ? TRUE : FALSE;
}

/****************************************************************************************/

VOID X11BM_DisposePM(struct bitmap_data *data)
{
    EnterFunc(bug("X11Gfx.BitMap::DisposePM()\n"));

    if (DRAWABLE(data))
    {
    	HostLib_Lock();

    	XCALL(XFreePixmap, GetSysDisplay(), DRAWABLE(data));
	XCALL(XFlush, GetSysDisplay());

    	HostLib_Unlock();
    }
    ReturnVoid("X11Gfx.BitMap::DisposePM");
}

/****************************************************************************************/

VOID X11BM_ClearPM(struct bitmap_data *data, HIDDT_Pixel bg)
{    
    XCALL(XSetForeground, data->display, data->gc, bg);
    XCALL(XFillRectangle, data->display, DRAWABLE(data), data->gc, 0, 0, data->width, data->height);    
}
