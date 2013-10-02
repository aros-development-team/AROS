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
    IPTR                        sbmflags;
};

// sbmflags bits 0 to 3 are reserved for the normal compositing flags.
#define STACKNODEB_HASALPHA      4
#define STACKNODEF_HASALPHA      (1 << STACKNODEB_HASALPHA)
#define STACKNODEB_VISIBLE       5
#define STACKNODEF_VISIBLE       (1 << STACKNODEB_VISIBLE)
#define STACKNODEB_DISPLAYABLE   6
#define STACKNODEF_DISPLAYABLE   (1 << STACKNODEB_DISPLAYABLE)

struct HIDDCompositorData
{
    struct GfxBase	        *GraphicsBase;
    struct IntuitionBase        *IntuitionBase;

    /* Bitmap to which all screen bitmaps are composited. Height/Width always 
       matches visible mode */
    OOP_Object                  *displaybitmap;
    OOP_Object                  *intermedbitmap;

    /* Pointer to actuall screen bitmap, result of last HIDD_Gfx_Show().
       Needed for graphics.library only. */
    OOP_Object                  *screenbitmap;

    /* Pointer to top bitmap on stack */
    OOP_Object                  *topbitmap;

    HIDDT_ModeID                screenmodeid;   /* ModeID of currently visible mode             */
    struct Rectangle            displayrect;     /* Dimensions of currently visible mode         */
    struct Region               *alpharegion;

    struct MinList              bitmapstack;
    struct SignalSemaphore      semaphore;

    struct Hook                 defaultbackfill;
    struct Hook                 *backfillhook;

    OOP_Object                  *gfx;           /* GFX driver object			        */
    OOP_Object		        *fb;		/* Framebuffer bitmap (if present)	        */
    OOP_Object                  *gc;            /* GC object used for drawing operations        */
    ULONG                       capabilities;
    ULONG                       flags;        
    BOOL                        modeschanged;   /* TRUE if new top bitmap has different mode than current screenmodeid */
};

#define COMPSTATEB_HASALPHA     0
#define COMPSTATEF_HASALPHA     (1 << COMPSTATEB_HASALPHA)

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define LOCK_COMPOSITOR_READ       { ObtainSemaphoreShared(&compdata->semaphore); }
#define LOCK_COMPOSITOR_WRITE      { ObtainSemaphore(&compdata->semaphore); }
#define UNLOCK_COMPOSITOR          { ReleaseSemaphore(&compdata->semaphore); }

extern OOP_AttrBase HiddCompositorAttrBase;
extern const struct OOP_InterfaceDescr Compositor_ifdescr[];

#endif /* _COMPOSITOR_INTERN_H */
