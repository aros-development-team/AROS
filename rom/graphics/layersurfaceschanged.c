/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Install a screen's list of composited layer surfaces and re-present.
          AROS private, for layers.library.
*/

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <hidd/gfx.h>
#include <proto/exec.h>

#include "graphics_intern.h"
#include "graphics_display.h"
#include "gfxfuncsupport.h"

#ifndef AROS_NO_COMPOSITED_LAYERS
#define AROS_NO_COMPOSITED_LAYERS 0
#endif

#if !AROS_NO_COMPOSITED_LAYERS
/*
 * The plane data behind a HIDD struct BitMap has no room left, so the surface
 * list of a screen bitmap is kept in a small private list of graphics.library,
 * keyed by the bitmap. HIDD_BMF_LAYERSURFACES in the bitmap's flags says a
 * node exists, so bitmaps without surfaces never search the list.
 */
static struct LayerSurfaceNode *gfx_LookupLayerSurfaceNode(struct BitMap *bitmap, struct GfxBase *GfxBase)
{
    struct LayerSurfaceNode *n;

    ForeachNode(&PrivGBase(GfxBase)->layersurfaces, n)
    {
        if (n->bitmap == bitmap)
            return n;
    }
    return NULL;
}

struct HIDD_CompositeRequest *gfx_FindLayerSurfaces(struct BitMap *bitmap, struct GfxBase *GfxBase)
{
    struct LayerSurfaceNode *n;
    struct HIDD_CompositeRequest *surfaces = NULL;

    if (!bitmap || !IS_HIDD_BM(bitmap) || !(HIDD_BM_FLAGS(bitmap) & HIDD_BMF_LAYERSURFACES))
        return NULL;

    ObtainSemaphoreShared(&PrivGBase(GfxBase)->layersurfaces_sema);
    n = gfx_LookupLayerSurfaceNode(bitmap, GfxBase);
    if (n)
        surfaces = n->surfaces;
    ReleaseSemaphore(&PrivGBase(GfxBase)->layersurfaces_sema);

    return surfaces;
}

/* Called by FreeBitMap() for a screen bitmap that still has a node */
void gfx_ForgetLayerSurfaces(struct BitMap *bitmap, struct GfxBase *GfxBase)
{
    struct LayerSurfaceNode *n;

    ObtainSemaphore(&PrivGBase(GfxBase)->layersurfaces_sema);
    n = gfx_LookupLayerSurfaceNode(bitmap, GfxBase);
    if (n)
    {
        Remove((struct Node *)n);
        FreeMem(n, sizeof(struct LayerSurfaceNode));
    }
    HIDD_BM_FLAGS(bitmap) &= ~HIDD_BMF_LAYERSURFACES;
    ReleaseSemaphore(&PrivGBase(GfxBase)->layersurfaces_sema);
}

/* Store (or, for NULL, drop) the list for this screen bitmap */
static void gfx_SetLayerSurfaces(struct BitMap *bitmap, struct HIDD_CompositeRequest *surfaces, struct GfxBase *GfxBase)
{
    struct LayerSurfaceNode *n;

    ObtainSemaphore(&PrivGBase(GfxBase)->layersurfaces_sema);
    n = (HIDD_BM_FLAGS(bitmap) & HIDD_BMF_LAYERSURFACES) ? gfx_LookupLayerSurfaceNode(bitmap, GfxBase) : NULL;
    if (surfaces)
    {
        if (!n)
        {
            n = AllocMem(sizeof(struct LayerSurfaceNode), MEMF_PUBLIC | MEMF_CLEAR);
            if (n)
            {
                n->bitmap = bitmap;
                AddHead((struct List *)&PrivGBase(GfxBase)->layersurfaces, (struct Node *)n);
                HIDD_BM_FLAGS(bitmap) |= HIDD_BMF_LAYERSURFACES;
            }
        }
        if (n)
            n->surfaces = surfaces;
    }
    else if (n)
    {
        Remove((struct Node *)n);
        FreeMem(n, sizeof(struct LayerSurfaceNode));
        HIDD_BM_FLAGS(bitmap) &= ~HIDD_BMF_LAYERSURFACES;
    }
    ReleaseSemaphore(&PrivGBase(GfxBase)->layersurfaces_sema);
}

#else /* AROS_NO_COMPOSITED_LAYERS */
/* ROM-size build: no screen bitmap ever carries layer surfaces */
struct HIDD_CompositeRequest *gfx_FindLayerSurfaces(struct BitMap *bitmap, struct GfxBase *GfxBase)
{
    return NULL;
}
void gfx_ForgetLayerSurfaces(struct BitMap *bitmap, struct GfxBase *GfxBase)
{
}
#endif /* !AROS_NO_COMPOSITED_LAYERS */

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(void, LayerSurfacesChanged,

/*  SYNOPSIS */
        AROS_LHA(struct BitMap *, screenbm, A0),
        AROS_LHA(struct HIDD_CompositeRequest *, surfaces, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 203, Graphics)

/*  FUNCTION
        Installs the list of composited layer surfaces of a screen bitmap
        and, when the screen is in the active View, has its display
        re-compose the affected area at once.

        The list (back to front through HIDD_CompositeRequest.Next, screen
        bitmap coordinates) is owned by layers.library. It is stored for the
        bitmap, so MakeVPort() copies it into the screen's HIDD_ViewPortData
        whenever the ViewPort is rebuilt, and the display's compositor takes
        a copy of the entries on every stack change. The Shape regions of
        the entries must stay valid until the next call for this bitmap.

    INPUTS
        screenbm - the screen's bitmap (a HIDD bitmap)
        surfaces - head of the new list, NULL when no composited layer is
                   left on the screen

    RESULT
        None.

    NOTES
        AROS private, for layers.library. Can be moved at any moment.
        Called with the screen's Layer_Info locked; the compositor only
        blits, so the lock order layers -> compositor is kept.

    SEE ALSO
        CompositeLayersSupported()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
#if AROS_NO_COMPOSITED_LAYERS
    /* ROM-size build: nothing composes layer surfaces (see layers_intern.h) */
#else

    struct gfxdisplay_data *display;
    struct ViewPort *vp;

    if (!screenbm || !IS_HIDD_BM(screenbm))
        return;

    gfx_SetLayerSurfaces(screenbm, surfaces, GfxBase);

    display = (struct gfxdisplay_data *)HIDD_BM_DRVDATA(screenbm);
    if (!display)
        return;

    /* Is the screen on display right now? Then update its ViewPortData and re-present. */
    ObtainSemaphore(GfxBase->ActiViewCprSemaphore);

    if (GfxBase->ActiView)
    {
        for (vp = GfxBase->ActiView->ViewPort; vp; vp = vp->Next)
        {
            struct ViewPortExtra *vpe;
            struct HIDD_ViewPortData *vpd;

            if (vp->Modes & VP_HIDE)
                continue;

            vpe = (struct ViewPortExtra *)GfxLookUp(vp);
            if (!vpe || !(vpd = VPE_DATA(vpe)))
                continue;

            if (vpd->Bitmap == HIDD_BM_OBJ(screenbm))
            {
                vpd->Surfaces = surfaces;
                HIDD_Display_SurfaceStackChanged(display->display_obj, vpd);
                break;
            }
        }
    }

    ReleaseSemaphore(GfxBase->ActiViewCprSemaphore);
#endif

    AROS_LIBFUNC_EXIT
} /* LayerSurfacesChanged */
