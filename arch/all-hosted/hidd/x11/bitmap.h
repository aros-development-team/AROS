/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 bitmap class, internal definitions
    Lang: english
*/

#include "bitmap_class.h"

#define GetSysDisplay() (data->display)
#define GetSysScreen()  (data->screen)
#define GetSysCursor()  (data->cursor)

#define DRAWABLE(data)      (data->drawable)
#define WINDRAWABLE(data)   (data->windowdrawable)

/* This structure is used as instance data for the bitmap class. */
struct bitmap_data
{
    Drawable        drawable;       /* The X11 object behind us        */
    Window          masterxwindow;
    Cursor          cursor;
    unsigned long   sysplanemask;
    Colormap        colmap;
    GC              gc;             /* !!! This is an X11 GC, NOT a HIDD gc */
    Display         *display;       /* Our X11 display            */
    int             screen;         /* Our X11 screen            */
    int             flags;          /* See below                */
    IPTR            width;          /* Cached size, used by Clear method    */
    IPTR            height;
    OOP_Object      *gfxhidd;       /* Cached owner, for ModeID switch    */
    Drawable        windowdrawable; /* Explic pointer to window drawable for BMDF_FRAMEBUFFER */
};

#define BMDF_COLORMAP_ALLOCED   1
#define BMDF_FRAMEBUFFER        2
#define BMDF_BACKINGSTORE       4

BOOL X11BM_InitFB(OOP_Class *cl, OOP_Object *o, struct TagItem *attrList);
VOID X11BM_InitEmptyCursor(struct bitmap_data *data);
VOID X11BM_DisposeFB(struct bitmap_data *data, struct x11_staticdata *xsd);
BOOL X11BM_SetMode(struct bitmap_data *data, HIDDT_ModeID modeid, struct x11_staticdata *xsd);
VOID X11BM_ClearFB(struct bitmap_data *data, HIDDT_Pixel bg);
VOID X11BM_ExposeFB(struct bitmap_data *data, WORD x, WORD y, WORD width, WORD height);

BOOL X11BM_InitPM(OOP_Class *cl, OOP_Object *o, struct TagItem *attrList);
VOID X11BM_DisposePM(struct bitmap_data *data);
VOID X11BM_ClearPM(struct bitmap_data *data, HIDDT_Pixel bg);
