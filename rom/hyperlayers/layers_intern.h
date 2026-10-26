/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Internal information for layers.library.
*/
#ifndef _LAYERS_INTERN_H_
#define _LAYERS_INTERN_H_

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/libraries.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <hidd/gfx.h>
#include <utility/utility.h>
#include <setjmp.h>
#include <dos/dos.h>   /* BPTR below */
#include <proto/alib.h> /* We redefine NewRectRegion() */

#include LC_LIBDEFS_FILE

#include "intregions.h"

LIBBASETYPE
{
    struct Library   	    lb_LibNode;

    APTR    	    	    lb_ClipRectPool;

    struct GfxBase *        lb_GfxBase;
    struct UtilityBase *    lb_UtilityBase;
};

/* Store library bases in our base, not in .bss. Why not ? */
#define GfxBase     (LIBBASE->lb_GfxBase)
#define UtilityBase (LIBBASE->lb_UtilityBase)

/*
 * Build-time option: one layer-sized backing bitmap (the "surface") per
 * smart refresh layer, the hidden ClipRects being views into it, instead
 * of one bitmap per hidden ClipRect. Hidden content is then never copied
 * between ClipRects, and clip regions and BeginUpdate() need no copying
 * at all. Costs one layer sized bitmap per smart refresh layer. Enable
 * with -DLAYERS_BACKING_SURFACE=1 (USER_CPPFLAGS in the mmakefile).
 */
#ifndef LAYERS_BACKING_SURFACE
#define LAYERS_BACKING_SURFACE 0
#endif

struct IntLayer
{
    struct Layer    lay;
    struct Hook	   *shapehook;
    IPTR	    window;	/* This is passed to shape hook. Comes from Intuition. */
    ULONG	    intflags;
    struct RastPort rp;		/* lay.rp */
    /*
     * The layer's own bitmap, layer relative: the surface of a composited
     * layer (LAYERCOMPOSITED, always), or the backing store of a smart
     * refresh layer (LAYERS_BACKING_SURFACE). NULL otherwise.
     */
    struct BitMap  *surface;
    WORD	    surfw, surfh;

    /*
     * What this layer hides from the layers behind it (what they subtract
     * from their VisibleRegion). Normally lay.visibleshape itself; for a
     * translucent composited layer (opacity < 255 or alpha surface) a
     * region of its own that stays empty, so the layers behind keep
     * rendering as if it were not there and the display blends it on top.
     * The layer's hit test (WhichLayer) and what it paints stay visibleshape.
     */
    struct Region  *occludeshape;

    /* Composited layers: the layer's entry in its screen's surface list */
    struct HIDD_CompositeRequest request;
    struct Region  *compshape;	/* request.Shape when the visible part is not the whole rectangle */
    UBYTE	    opacity;	/* 255 = opaque */
    BOOL	    alphasurface;
    ULONG	    alphaformat;	/* cybergraphics PIXFMT_#? of an alpha surface (for reallocation) */
    ULONG	    surfbytes;	/* bytes of the surface counted in lie_SurfaceBytes (composited only) */

    /* LA_InputShape: own copy, layer relative; NULL: input where it is seen */
    struct Region  *inputshape;
};

#define IL(x) ((struct IntLayer *)(x))

/* Standard layer priorities */
#define ROOTPRIORITY		0
#define BACKDROPPRIORITY	10
#define UPFRONTPRIORITY		20

#define INTFLAG_AVOID_BACKFILL 1
#define INTFLAG_TRANSLUCENT    2	/* composited, blended: occludes nothing */

#define IS_COMPOSITED(l)  (0 != ((l)->Flags & LAYERCOMPOSITED))
#define IS_TRANSLUCENT(l) (0 != (IL(l)->intflags & INTFLAG_TRANSLUCENT))
#define OCCLUDESHAPE(l)   (IL(l)->occludeshape)

/*
 * Policy: does an opaque composited layer occlude the layers behind it
 * (they back up and restore their pixels as under any opaque layer, the
 * design in hyperlayers-composition-study.md 7.3), or is it treated like a
 * translucent one (the layers behind keep painting into the screen bitmap,
 * which the compositor does not show there, and depth or position changes
 * of the composited layer copy no pixels at all)? Both are correct with
 * the compositor; 1 is the documented design.
 */
#ifndef LAYERS_COMPOSITED_OCCLUDES
#define LAYERS_COMPOSITED_OCCLUDES 1
#endif

/*
 * Memory budget of composited layer surfaces, per Layer_Info (screen):
 * their surfaces together may take at most LAYERS_SURFACE_BUDGET_FACTOR
 * times the screen bitmap, and a surface is never allocated when it would
 * leave less than LAYERS_SURFACE_MINFREE of the largest free memory block.
 * A layer that does not fit is created as an ordinary smart refresh layer,
 * and a composited layer whose growth (SizeLayer()) does not fit is turned
 * into one (_UncompositeLayer). See <graphics/layersext.h>.
 */
#ifndef LAYERS_SURFACE_BUDGET_FACTOR
#define LAYERS_SURFACE_BUDGET_FACTOR 4
#endif
#ifndef LAYERS_SURFACE_MINFREE
#define LAYERS_SURFACE_MINFREE (4UL * 1024 * 1024)
#endif

struct LayerInfo_extra
{
#if 0
    ULONG          lie_ReturnAddr;     // used by setjmp/longjmp, equals jmp_buf
    ULONG          lie_Regs[12];       // D2-D7/A2-SP
#else
    jmp_buf        lie_JumpBuf;
#endif
    struct MinList lie_ResourceList;
    UBYTE          lie_pad[4];

    /*
     * Composited layers of this Layer_Info: how many there are, the head of
     * their back to front list of surface requests as last handed to
     * graphics.library (LayerSurfacesChanged()) and the screen bitmap it
     * was handed for. Intuition screens get a LayerInfo_extra only when
     * their first composited layer is created.
     */
    LONG           lie_NumComposited;
    struct HIDD_CompositeRequest *lie_Surfaces;
    struct BitMap *lie_ScreenBM;
    ULONG          lie_SurfaceBytes;	/* bytes of all composited surfaces (budget, see above) */
};

#define LIE(li) ((struct LayerInfo_extra *)(li)->LayerInfo_extra)

/*
 * Hand the Layer_Info's surface list to the display at the end of a layer
 * operation. Nothing is done (one test) when it never had a composited layer.
 */
#define UPDATE_SURFACES(li) \
    do { \
        if (NULL != LIE(li) && (0 != LIE(li)->lie_NumComposited || NULL != LIE(li)->lie_Surfaces)) \
            _UpdateSurfaces(li, LayersBase); \
    } while (0)

/*
 * These are special types of ResData resources. If layers finds one of
 * these values in ResData->Size, it performs some special handling to
 * properly dispose of the allocated Region or BitMap, respectively
 * (throught DisposeRegion or FreeBitMap). In all other cases,
 * ResData->Size is an argument for a freemem operation.
 */
#define RD_REGION -1
#define RD_BITMAP -2

struct ResData
{
    void *ptr;
    ULONG Size;
};

struct ResourceNode
{
    struct Node	    rn_Link;
    struct ResData *rn_FirstFree;
    LONG            rn_FreeCnt;
    struct ResData  rn_Data[48];
};


/*
** The smart refresh flag is set for super bitmap as well as smart refresh
** layers 
*/
#define IS_SIMPLEREFRESH(l) (0 != ((l)->Flags & LAYERSIMPLE))
#define IS_SMARTREFRESH(l)  (LAYERSMART == ((l)->Flags & (LAYERSMART|LAYERSUPER)))
#define IS_SUPERREFRESH(l)  (0 != ((l)->Flags & LAYERSUPER))

int _MoveLayerBehind(struct Layer *l,
                     struct Layer *lfront,
                     LIBBASETYPEPTR LayersBase);
int _MoveLayerToFront(struct Layer * l,
                      struct Layer * lbehind,
                      LIBBASETYPEPTR LayersBase);

#define NewRectRegion(MinX, MinY, MaxX, MaxY) _NewRectRegion(MinX, MinY, MaxX, MaxY, LIBBASE->lb_GfxBase)

#endif /* _LAYERS_INTERN_H */
