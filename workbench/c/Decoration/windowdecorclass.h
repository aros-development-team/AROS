/*
    Copyright  2011-2026, The AROS Development Team.
*/

#ifndef WINDOWDECORCLASS_H
#define WINDOWDECORCLASS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>

#include <libraries/decortheme.h>

struct CachedPropGadget
{
    /* This is pregenerate bitmap matching state saved in rest of fields */
    struct BitMap   *bm; 

    UWORD           width;
    UWORD           height;
    UWORD           knobwidth;
    UWORD           knobheight;
    UWORD           knobx;
    UWORD           knoby;
    ULONG           windowflags;
    ULONG           gadgetflags;
};

struct CachedTitleBar
{
    /* This is pregenerate bitmap matching state saved in rest of fields */
    struct BitMap   *bm;

    UWORD           width;
    UWORD           height;
    ULONG           windowflags;
    STRPTR          title;
    ULONG           titlelen;
};

struct CachedTitleBarShape
{
    /* This is pregenerated Region shape matching state saved in rest of fields */
    struct Region   *shape;

    UWORD           width;
    UWORD           height;
    ULONG           windowflags;
};

struct  WindowData
{
    /* Per-screen theme instance shared with the ScreenData (not owned) */
    struct DecorThemeScreen *dts;

    struct DecorImage *di;

    struct DecorImage *img_size;
    struct DecorImage *img_close;
    struct DecorImage *img_depth;
    struct DecorImage *img_zoom;
    struct DecorImage *img_up;
    struct DecorImage *img_down;
    struct DecorImage *img_left;
    struct DecorImage *img_right;
    struct DecorImage *img_mui;
    struct DecorImage *img_popup;
    struct DecorImage *img_snapshot;
    struct DecorImage *img_iconify;
    struct DecorImage *img_lock;
    struct DecorImage *img_winbar_normal;
    struct DecorImage *img_border_normal;
    struct DecorImage *img_border_deactivated;
    struct DecorImage *img_verticalcontainer;
    struct DecorImage *img_verticalknob;
    struct DecorImage *img_horizontalcontainer;
    struct DecorImage *img_horizontalknob;

    LONG                ActivePen;
    LONG                DeactivePen;

    WORD   closewidth, depthwidth, zoomwidth;
    BOOL   truecolor;
    
    /* Cached bitmaps used to improve speed of redrawing of decorated window */
    struct CachedPropGadget     vert;
    struct CachedPropGadget     horiz;
    struct CachedTitleBar       tbar;
    struct CachedTitleBarShape  tbarshape;
};

#define WDA_DecorImages     0x30003
#define WDA_DecorConfig     0x30004
#define WDA_DecorTheme      0x30005

struct IClass * MakeWindowDecorClass();
#endif
