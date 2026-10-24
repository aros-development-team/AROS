/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.

    Desc: Private definitions for the base display compositor class.
*/

#ifndef GFX_COMPOSITOR_H
#define GFX_COMPOSITOR_H

#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <graphics/gfx.h>
#include <graphics/regions.h>
#include <utility/hooks.h>

/*
 * A composited layer surface shown inside a screen: a private copy of the
 * HIDD_CompositeRequest handed over by graphics.library, plus the display
 * relative region it covers after clipping to its screen.
 */
struct SurfaceNode
{
    struct MinNode               n;         /* back to front                              */
    struct HIDD_CompositeRequest req;       /* Dest/Shape are screen bitmap relative      */
    struct Region               *region;    /* visible part, display relative             */
    BOOL                         opaque;    /* blitted, and hides the screen under it     */
};

/* One bitmap on the display, in Z order (first = frontmost) */
struct StackBitMapNode
{
    struct MinNode  n;
    OOP_Object      *bm;
    struct Region   *screenregion;      /* visible part, display relative */
    struct Region   *paintregion;       /* screenregion minus opaque surfaces */
    SIPTR           leftedge;           /* offset on the display          */
    SIPTR           topedge;
    IPTR            sbmflags;           /* COMPF_* of this screen + STACKNODEF_* */
    struct Hook     *prealphacomphook;  /* from HIDD_ViewPortData.PreAlphaHook */
    struct MinList  surfaces;           /* struct SurfaceNode, back to front */
    BOOL            hassurfaces;
    LONG            plane;              /* scan-out plane showing this screen, -1 = composed */
    /*
     * Geometry: the screen bitmap is scrw x scrh pixels and takes dstw x dsth
     * pixels on the display. They differ (scaled) only with ModePolicy_Native,
     * where every screen is scaled to the display's own mode; leftedge/topedge
     * stay in screen pixels (that is what Intuition's drag code works in).
     */
    LONG            scrw, scrh;
    LONG            dstw, dsth;
    BOOL            scaled;
};

/* sbmflags bits 0 to 8 are the COMPF_* composition flags of the screen */
#define STACKNODEB_VISIBLE       16
#define STACKNODEF_VISIBLE       (1 << STACKNODEB_VISIBLE)
#define STACKNODEB_DISPLAYABLE   17
#define STACKNODEF_DISPLAYABLE   (1 << STACKNODEB_DISPLAYABLE)

struct HIDDCompositorData
{
    struct class_static_data    *compcsd;       /* module static data (for the method bases) */
    struct GfxBase              *GraphicsBase;  /* opened lazily, after graphics.library is up */
    struct Library              *dosbase;       /* opened lazily, for the prefs variable only  */

    ULONG                       capabilities;   /* COMPF_* currently enabled (prefs)          */
    ULONG                       flags;          /* COMPSTATEF_*                               */
    ULONG                       modepolicy;     /* vHidd_Compositor_ModePolicy_*              */
    BOOL                        can_compose;    /* the display has a truecolor mode           */
    BOOL                        prefsread;      /* ENV:SYS/compositor.prefs has been read     */

    /* Bitmap to which all screen bitmaps are composited. Height/Width always
       matches visible mode */
    OOP_Object                  *displaybitmap;
    OOP_Object                  *intermedbitmap;

    /* Pointer to actual screen bitmap, result of last HIDD_Display_Show(). */
    OOP_Object                  *screenbitmap;
    OOP_Object                  *passthroughbm;  /* bitmap shown in passthrough (first screen without a plane), NULL otherwise */

    /* ModePolicy_Native: the display's own (largest) mode, looked up once */
    BOOL                        nativeknown;
    ULONG                       nativew, nativeh;

    /* Row buffers for scaled blits (grown on demand, freed in Dispose) */
    ULONG                       *scalesrc, *scaledst;
    UWORD                       *scalemap;
    ULONG                       scalesrclen, scaledstlen;

    /* Pointer to top bitmap on stack */
    OOP_Object                  *topbitmap;

    struct Rectangle            displayrect;     /* Dimensions of currently visible mode         */
    struct Region               *alpharegion;

    struct MinList              bitmapstack;
    struct SignalSemaphore      semaphore;

    struct Hook                 *backfillhook;

    OOP_Object                  *gfx;           /* GFX driver object                            */
    OOP_Object                  *display;       /* Display object this compositor serves        */
    OOP_Object                  *dmenum;        /* Display mode enumerator of the display       */
    OOP_Object                  *gc;            /* GC object used for drawing operations        */

    HIDDT_ModeID                displaymode;    /* ModeID of currently visible mode             */
    UBYTE                       displaydepth;

    struct Hook                 defaultbackfill;
    BOOL                        modeschanged;   /* TRUE if new top bitmap has different mode than current displaymode */

    /*
     * Scan-out planes of the display (aHidd_Display_PlaneCount, GetPlaneInfo),
     * fetched at the first stack change. With more than the primary plane,
     * the frontmost screens that fit a plane are scanned out directly and only
     * the rest is composed into the primary.
     */
    ULONG                       planecount;
    ULONG                       planealloc;     /* entries allocated for the tables below   */
    struct HIDD_PlaneInfo       *planes;        /* planecount entries                       */
    struct HIDD_PlaneState      *planestates;   /* planecount entries, what was committed   */
    BOOL                        planesknown;
    BOOL                        noplanes;       /* prefs NOPLANES: never use scan-out planes */
};

#define COMPSTATEB_HASALPHA     0
#define COMPSTATEF_HASALPHA     (1 << COMPSTATEB_HASALPHA)
#define COMPSTATEB_DEEPLUT      1
#define COMPSTATEF_DEEPLUT      (1 << COMPSTATEB_DEEPLUT)

#endif /* GFX_COMPOSITOR_H */
