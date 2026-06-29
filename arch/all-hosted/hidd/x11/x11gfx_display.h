#ifndef X11GFX_DISPLAY_H
#define X11GFX_DISPLAY_H

/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: X11 Gfx display class data.
    Lang: English.
*/

#define CLID_Hidd_Display_X11	"hidd.display.x11"
#define IID_Hidd_Display_X11	"hidd.display.x11"

struct X11DisplayData
{
    Display	*display;
    int		 screen;
    int		 depth;
    Colormap	 colmap;
    Cursor	 cursor;

    /* baseclass for CreateObject */
    OOP_Class	*basebm;
};

#endif /* X11GFX_DISPLAY_H */
