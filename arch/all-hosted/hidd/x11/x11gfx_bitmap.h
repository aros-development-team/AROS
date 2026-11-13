/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: X11 bitmap class, internal definitions
*/

#include "x11gfx_bitmapclass.h"

#define GetSysDisplay() (data->display)
#define GetSysScreen()  (data->screen)
#define GetSysCursor()  (data->cursor)

#define DRAWABLE(data)      (data->drawable)
#define WINDRAWABLE(data)   (data->windowdrawable)

/*
 * An emulated overlay scan-out plane: a child window of the framebuffer window
 * whose background pixmap is the scanned bitmap. Owned by the display class;
 * the bitmap bound to it keeps a pointer so that UpdateRect() can refresh the
 * window and Dispose() can detach.
 */
struct x11_plane
{
    Window           win;
    OOP_Object      *bm;        /* bitmap scanned out by this plane, NULL = off */
    BOOL             mapped;
    /*
     * Scaled plane (XRender): the bitmap is rendered scaled into 'scaled', a
     * pixmap of the plane's size on the display, which is the window's
     * background; a refresh re-renders the changed part and clears it.
     */
    BOOL             isscaled;
    Pixmap           scaled;
    unsigned long    srcpic, dstpic;    /* XRender Pictures (XIDs); Xrender.h is only seen by the display class */
    LONG             srcw, srch, dstw, dsth;
};

/* x11gfx_displayclass.c: plane helpers used by the bitmap class */
VOID X11Plane_Refresh(Display *d, struct x11_plane *p, LONG x, LONG y, LONG w, LONG h);
VOID X11Plane_Detach(Display *d, struct x11_plane *p);

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
    OOP_Object      *dmenum;        /* Cached display mode enumerator      */
    Drawable        windowdrawable; /* Explicit pointer to window drawable for BMDF_FRAMEBUFFER */
    struct x11_plane *plane;        /* Overlay plane scanning this bitmap out, or NULL */
};

#define BMDF_COLORMAP_ALLOCED   1
#define BMDF_FRAMEBUFFER        2
#define BMDF_BACKINGSTORE       4

BOOL X11BM_InitFB(OOP_Class *cl, OOP_Object *o, struct TagItem *attrList);
BOOL X11BM_NotifyFB(OOP_Class *cl, OOP_Object *o);
VOID X11BM_InitEmptyCursor(struct bitmap_data *data);
VOID X11BM_DisposeFB(struct bitmap_data *data, struct x11_staticdata *xsd);
BOOL X11BM_SetMode(struct bitmap_data *data, HIDDT_ModeID modeid, struct x11_staticdata *xsd);
VOID X11BM_ClearFB(struct bitmap_data *data, HIDDT_Pixel bg);
VOID X11BM_ExposeFB(struct bitmap_data *data, WORD x, WORD y, WORD width, WORD height);

BOOL X11BM_InitPM(OOP_Class *cl, OOP_Object *o, struct TagItem *attrList);
VOID X11BM_DisposePM(struct bitmap_data *data);
VOID X11BM_ClearPM(struct bitmap_data *data, HIDDT_Pixel bg);
