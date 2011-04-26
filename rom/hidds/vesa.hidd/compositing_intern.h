#ifndef _COMPOSITING_INTERN_H
#define _COMPOSITING_INTERN_H
/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "compositing.h"

#include <exec/lists.h>

struct _Rectangle
{
    WORD MinX;
    WORD MinY;
    WORD MaxX;
    WORD MaxY;
};

struct StackBitMapNode
{
    struct Node n;
    OOP_Object *bm;
    struct _Rectangle screenvisiblerect;
    BOOL isscreenvisible;
};

struct PrivateData
{
    HIDDT_ModeID screenmodeid;
    struct _Rectangle screenrect;

    struct List bitmapstack;

    struct SignalSemaphore semaphore;

    OOP_Object *gfx;           /* GFX driver object */
    OOP_Object *gc;            /* GC object used for drawing operations */

    void (*refresh_hook)(OOP_Object *gfx, OOP_Object *bm,
        LONG x1, LONG y1, LONG x2, LONG y2);

    WORD first_used_line;
};

#define METHOD(base, id, name) \
    base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, \
    struct p ## id ## _ ## name *msg)

#endif /* _COMPOSITING_INTERN_H */
