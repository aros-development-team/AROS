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
 * A client surface (video overlay, 3D colour buffer) attached through
 * moHidd_Compositor_AttachSurface. It outlives the screen's stack node: the
 * screen may leave the display and come back while the client stays attached,
 * so the registry entry lives in HIDDCompositorData.clientsurfaces (attach
 * order) and is linked to a SurfaceNode on the screen's list whenever the
 * screen is on the stack.
 */
struct ClientSurface
{
    struct MinNode                n;
    OOP_Object                   *screenbm;  /* the screen it was attached to              */
    struct HIDD_CompositeRequest *req;       /* the client's own request, never copied     */
    struct Hook                  *hook;      /* surface hook, may be NULL                  */
    struct SurfaceNode           *sn;        /* node on the screen's list, NULL when the
                                              * screen is not on the stack                */
    LONG                          hookplane; /* plane last reported to the hook, -1 = none */
    ULONG                         firing;    /* hook calls collected but not yet made     */
    struct Task                  *waiter;    /* DetachSurface waiting for firing == 0     */
};

/*
 * A surface shown inside a screen: a composited layer (a private copy of the
 * HIDD_CompositeRequest handed over by graphics.library) or a client surface
 * (the client's request, pointed at), plus the display relative region it
 * covers after clipping to its screen, and the scan-out plane it holds.
 */
struct SurfaceNode
{
    struct MinNode               n;         /* back to front                              */
    struct HIDD_CompositeRequest req;       /* layer surface: the copy; Dest/Shape are
                                             * screen bitmap relative                    */
    struct HIDD_CompositeRequest *creq;     /* client surface: the client's request,
                                             * NULL for a layer surface                  */
    struct ClientSurface        *client;    /* registry entry, NULL for a layer surface   */
    struct Region               *region;    /* visible part, display relative             */
    BOOL                         opaque;    /* hides the screen under it (blitted opaque,
                                             * or shown by a plane)                      */
    ULONG                        videofmt;  /* aHidd_BitMap_VideoFormat of the bitmap     */
    BOOL                         scanout;   /* aHidd_BitMap_Scanout of the bitmap         */
    BOOL                         candidate; /* may take a plane in this pass              */
    BOOL                         denied;    /* lost its plane in this pass's test loop    */
    LONG                         plane;     /* scan-out plane showing it, -1 = none       */
    LONG                         zorder;    /* z the plane was given                      */
    ULONG                        pflags;    /* PSF_* of the plane state                   */
    struct Rectangle             psrc;      /* plane state: part of the bitmap shown      */
    struct Rectangle             pdest;     /* plane state: where on the display          */
};

/* The request a SurfaceNode shows */
#define SNREQ(sn) ((sn)->creq ? (sn)->creq : &(sn)->req)

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
    struct MinList  surfaces;           /* struct SurfaceNode, back to front: layer surfaces, then client surfaces */
    BOOL            hassurfaces;        /* the list is not empty */
    BOOL            haslayersurfaces;   /* composited layers (always blitted) are on it */
    LONG            plane;              /* scan-out plane showing this screen, -1 = composed */
    LONG            zorder;             /* z the plane was given */
    LONG            lastplane;          /* plane before the current re-plan */
    BOOL            promoted;           /* plane assignment in progress: gets a plane */
    BOOL            planedenied;        /* lost its plane in this pass's test loop */
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

#include "gfx_palette.h"

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

    /* A rectangle with layer surfaces in it is composed here and copied to
       displaybitmap in one blit, so that the display never shows the
       backdrop without the surfaces over it (see HIDDCompositorTarget()).
       Display sized, made on first use and again after a mode change,
       freed with the display bitmap. surfacesem: one user at a time, as
       updates arrive under the shared compositor lock. */
    OOP_Object                  *surfacebitmap;
    struct Rectangle            surfacebmrect;
    ULONG                       surfacebmdepth, surfacebmmode;
    struct SignalSemaphore      surfacesem;

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
    struct GfxPaletteWorker      palette;

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
    struct HIDD_PlaneState      *commitstates;  /* planecount entries, the set handed to the driver */
    BOOL                        planesknown;
    BOOL                        noplanes;       /* prefs NOPLANES: never use scan-out planes */
    ULONG                       colorkey;       /* aHidd_Display_ColorKey, 0x00RRGGBB         */

    /*
     * Client surfaces (AttachSurface) of every screen, attach order. Their
     * hooks are called with the compositor lock dropped; hooksem guards the
     * per-surface call counters that DetachSurface waits on (see the hook
     * firing rule in gfx_compositorclass.c).
     */
    struct MinList              clientsurfaces; /* struct ClientSurface                       */
    ULONG                       nclientsurfaces;
    struct SignalSemaphore      hooksem;
};

#define COMPSTATEB_HASALPHA     0
#define COMPSTATEF_HASALPHA     (1 << COMPSTATEB_HASALPHA)
#define COMPSTATEB_DEEPLUT      1
#define COMPSTATEF_DEEPLUT      (1 << COMPSTATEB_DEEPLUT)

#endif /* GFX_COMPOSITOR_H */
