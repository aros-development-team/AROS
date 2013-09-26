#ifndef _COMPOSITING_INTERN_H
#define _COMPOSITING_INTERN_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: compositing_intern.h 38695 2011-05-15 18:21:22Z deadwood $
*/

#include "compositing.h"

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

struct HIDDCompositingData
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

#define LOCK_COMPOSITING_READ       { ObtainSemaphoreShared(&compdata->semaphore); }
#define LOCK_COMPOSITING_WRITE      { ObtainSemaphore(&compdata->semaphore); }
#define UNLOCK_COMPOSITING          { ReleaseSemaphore(&compdata->semaphore); }

extern OOP_AttrBase HiddCompositingAttrBase;
extern const struct OOP_InterfaceDescr Compositing_ifdescr[];

#define IS_COMPOSITOR_ATTR(attr, idx)  \
        ( ( ( idx ) = (attr) - HiddCompositingAttrBase) < num_Hidd_Compositing_Attrs)

#endif /* _COMPOSITING_INTERN_H */
