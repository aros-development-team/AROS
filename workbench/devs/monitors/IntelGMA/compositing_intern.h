#ifndef _COMPOSITING_INTERN_H
#define _COMPOSITING_INTERN_H
/*
    Copyright � 2010, The AROS Development Team. All rights reserved.
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
    struct Node         n;
    OOP_Object *        bm;
    struct _Rectangle   screenvisiblerect;
    BOOL                isscreenvisible;
    LONG                displayedwidth;
    LONG                displayedheight;
};

struct HIDDCompositingData
{
    OOP_Object              *screenbitmap;
	OOP_Object              *directbitmap; 
    HIDDT_ModeID            screenmodeid;
    struct _Rectangle       screenrect;

    struct List             bitmapstack;
    
    struct SignalSemaphore  semaphore;
    
    OOP_Object              *gfx;           /* GFX driver object */
    OOP_Object              *gc;            /* GC object used for drawing operations */
};

extern const struct OOP_InterfaceDescr Compositing_ifdescr[];

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)
  
//#define SD(cl)                      (&BASE(cl->UserData)->sd)

#define LOCK_COMPOSITING_READ       { ObtainSemaphoreShared(&compdata->semaphore); }
#define LOCK_COMPOSITING_WRITE      { ObtainSemaphore(&compdata->semaphore); }
#define UNLOCK_COMPOSITING          { ReleaseSemaphore(&compdata->semaphore); }

#endif /* _COMPOSITING_INTERN_H */
