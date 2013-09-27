#ifndef _COMPOSITOR_INTERN_H
#define _COMPOSITOR_INTERN_H

/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "compositor.h"

#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <graphics/gfx.h>

struct StackBitMapNode
{
    struct MinNode              n;
    OOP_Object	                *bm;
    struct Region               *screenregion;
    struct Rectangle            screenvisiblerect;      /* Visible part */
    SIPTR	                leftedge;		/* Offset */
    SIPTR	                topedge;
    ULONG                       sbmflags;
};

#define STACKNODE_ALPHA         (1 << 0)
#define STACKNODE_VISIBLE       (1 << 1)

struct HIDDCompositorData
{
    struct GfxBase	        *GraphicsBase;
    /* Bitmap to which all screen bitmaps are composited. Height/Width always 
       matches visible mode */
    OOP_Object                  *compositedbitmap;

    /* Pointer to actuall screen bitmap, result of last HIDD_Gfx_Show().
       Needed for graphics.library only. */
    OOP_Object                  *screenbitmap;

    /* Pointer to top bitmap on stack */
    OOP_Object                  *topbitmap;

    HIDDT_ModeID                screenmodeid;   /* ModeID of currently visible mode             */
    struct Rectangle            screenrect;     /* Dimensions of currently visible mode         */

    struct MinList              bitmapstack;
    struct SignalSemaphore      semaphore;

    struct Region               *alpharegion;

    struct Hook                 defaultbackfill;
    struct Hook                 *backfillhook;

    OOP_Object                  *gfx;           /* GFX driver object			        */
    OOP_Object		        *fb;		/* Framebuffer bitmap (if present)	        */
    OOP_Object                  *gc;            /* GC object used for drawing operations        */
    ULONG                       capabilities;
    BOOL                        modeschanged;   /* TRUE if new top bitmap has different mode than current screenmodeid */
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define LOCK_COMPOSITOR_READ       { ObtainSemaphoreShared(&compdata->semaphore); }
#define LOCK_COMPOSITOR_WRITE      { ObtainSemaphore(&compdata->semaphore); }
#define UNLOCK_COMPOSITOR          { ReleaseSemaphore(&compdata->semaphore); }

extern OOP_AttrBase HiddCompositorAttrBase;
extern const struct OOP_InterfaceDescr Compositor_ifdescr[];

#endif /* _COMPOSITOR_INTERN_H */
