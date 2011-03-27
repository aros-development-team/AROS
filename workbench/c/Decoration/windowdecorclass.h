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

    struct  RastPort    *rp;
    UWORD               w, h;
    LONG                ActivePen;
    LONG                DeactivePen;

    WORD   closewidth, depthwidth, zoomwidth;
    BOOL   truecolor;
};

#define WDA_Configuration   0x30002
#define WDA_DecorImages     0x30003

struct IClass * MakeWindowDecorClass();
#endif
