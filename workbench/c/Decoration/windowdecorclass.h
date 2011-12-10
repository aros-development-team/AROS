/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef WINDOWDECORCLASS_H
#define WINDOWDECORCLASS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>

#include "newimage.h"

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
    struct NewImage *ni;

    struct NewImage *img_size;
    struct NewImage *img_close;
    struct NewImage *img_depth;
    struct NewImage *img_zoom;
    struct NewImage *img_up;
    struct NewImage *img_down;
    struct NewImage *img_left;
    struct NewImage *img_right;
    struct NewImage *img_mui;
    struct NewImage *img_popup;
    struct NewImage *img_snapshot;
    struct NewImage *img_iconify;
    struct NewImage *img_lock;
    struct NewImage *img_winbar_normal;
    struct NewImage *img_border_normal;
    struct NewImage *img_border_deactivated;
    struct NewImage *img_verticalcontainer;
    struct NewImage *img_verticalknob;
    struct NewImage *img_horizontalcontainer;
    struct NewImage *img_horizontalknob;

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

struct IClass * MakeWindowDecorClass();
#endif
