/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <exec/types.h>

#include "layers_intern.h"
#include "basicfuncs.h"
/*****************************************************************************

    NAME */
#include <proto/layers.h>

	AROS_LH3(void, DoHookClipRects,

/*  SYNOPSIS */
	AROS_LHA(struct Hook      *, hook ,  A0),
	AROS_LHA(struct RastPort  *, rport,  A1),
	AROS_LHA(struct Rectangle *, rect ,  A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 36, Layers)

/*  FUNCTION

    INPUTS
        hook  - pointer to the hook to be called for the cliprects of
                the given layer
               
        rport - pointer to the rastport where the layers upon which the
                hook is to be called
        
        rect  - no operation is allowed outside this rectangle. If a layer
                is bigger than this rectangle only operations in the
                common area are allowed.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{ /* sv */
    AROS_LIBFUNC_INIT

    struct Layer     *L;

    D(bug("[Layers] %s(hook @ 0x%p, rport @ $%p, rect @ $%p)\n", __func__, hook, rport, rect));

    /* if the hook is LAYERS_NOBACKFILL, there is nothing to do here. */
    if(hook == (struct Hook *)LAYERS_NOBACKFILL)
    {
	D(bug("[Layers] %s: NOBACKFILL\n", __func__));
	return;
    }

    L = rport -> Layer;
    /* does this rastport have a layer?? */
    if( NULL == L )
    {
	D(bug("[Layers] %s: No Layer\n", __func__));

	/* non-layered rastport */

	/* You MUST supply a rect to clip the hook's actions! */
	_CallLayerHook(hook, rport, NULL, rect, rect->MinX, rect->MinY, LayersBase);
    }
    else
    {
	struct Rectangle boundrect;

	/* layered rastport */

	/* The input rectangle is relative to the upper left corner of the
	 * layer's rastport. */

	LockLayer(0, L);

	D(bug("[Layers] %s: Layer > %d,%d -> %d,%d\n", __func__, L->bounds.MinX - L->Scroll_X, L->bounds.MinY - L->Scroll_Y, L->bounds.MaxX - L->Scroll_X, L->bounds.MaxY - L->Scroll_Y);)

	boundrect.MinX = rect->MinX + L->bounds.MinX - L->Scroll_X;
	boundrect.MinY = rect->MinY + L->bounds.MinY - L->Scroll_Y;
	boundrect.MaxX = rect->MaxX + L->bounds.MinX - L->Scroll_X;
	boundrect.MaxY = rect->MaxY + L->bounds.MinY - L->Scroll_Y;

	D(bug("[Layers] %s:         %d,%d -> %d,%d\n", __func__, boundrect.MinX, boundrect.MinY, boundrect.MaxX, boundrect.MaxY);)

	/* first check whether this layer is to be considered at all */
	if (!(boundrect.MinX > L->bounds.MaxX ||
              boundrect.MinY > L->bounds.MaxY ||
              boundrect.MaxX < L->bounds.MinX ||
              boundrect.MaxY < L->bounds.MinY))
	{
	    /* yes, that's a layer to be considered */
	    /* I want nobody else to interrupt me while I call the hook for this layer */
	    struct ClipRect * CR;


	    CR = L->ClipRect; /* must not read this before LockLayer!! */

	    /* process all ClipRects of this layer */
	    while (NULL != CR)
	    {
        	D(bug("[Layers] %s: CR @ 0x%p\n", __func__, CR));
        	/* The hook will be called for all visible cliprects, and
        	   for hidden cliprects belonging to smart or superbitmap
        	   layers =>
        	   Hidden cliprects belonging to simple layers are ignored.
        	*/

        	if (!(NULL != CR->lobs && 
        	      0    != (L->Flags & LAYERSIMPLE)) )
        	{
        	    struct Rectangle bounds;
        	    /* That's a ClipRect to visit, if it's inside the given rectangle */
        	    /* Generate the bounds rectangle. This rectangle shows the part
        	       of the clipRect that is supposed to be changed. So it might get
        	       the coordinates of the ClipRect, but it can also be smaller. */

        	    D(bug("[Layers] %s: CR > %d,%d -> %d,%d\n", __func__, CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY);)

        	    bounds.MinX = (boundrect.MinX > CR->bounds.MinX) ? boundrect.MinX 
                                                        	     : CR->bounds.MinX;
        	    bounds.MinY = (boundrect.MinY > CR->bounds.MinY) ? boundrect.MinY 
                                                        	     : CR->bounds.MinY;
        	    bounds.MaxX = (boundrect.MaxX < CR->bounds.MaxX) ? boundrect.MaxX 
                                                        	     : CR->bounds.MaxX;
        	    bounds.MaxY = (boundrect.MaxY < CR->bounds.MaxY) ? boundrect.MaxY 
                                                        	     : CR->bounds.MaxY;

        	    D(bug("[Layers] %s:      %d,%d -> %d,%d\n", __func__, bounds.MinX, bounds.MinY, bounds.MaxX, bounds.MaxY);)

        	    /* Is the cliprect inside the bounds?... */
        	    if (bounds.MinX <= bounds.MaxX && bounds.MinY <= bounds.MaxY)
        	    {
        		struct BitMap * bm;
        		WORD offsetX = bounds.MinX - L->bounds.MinX + L->Scroll_X; /* + scrollx is correct! */
        		WORD offsetY = bounds.MinY - L->bounds.MinY + L->Scroll_Y; /* + scrolly is correct! */

        		/* Call the hook for the rectangle given by bounds. */

        		/* If the ClipRect is hidden, then this might get special..., but
        		   only for non-simple layers, which are already ignored by an if
			   further above */
        		if (NULL != CR->lobs)
			{
        		    bm = rport->BitMap;

        		    /* it's a smart layer -: */
        		    if (0 != (L->Flags & LAYERSUPER))
			    {
                		/* it's a superbitmap layer */

                		D(bug("[Layers] %s:   SuperBitMap\n", __func__));

                		bounds.MinX -= ( L->bounds.MinX + L->Scroll_X );
                		bounds.MinY -= ( L->bounds.MinY + L->Scroll_Y );
                		bounds.MaxX -= ( L->bounds.MinX + L->Scroll_X );
                		bounds.MaxY -= ( L->bounds.MinY + L->Scroll_Y );
                		rport->BitMap = L->SuperBitMap;
			    }
        		    else
			    {
                		/* It's not a superbitmap, so it must be a hidden layer.
                		 * The hook has to blit into the hidden cliprect's bitmap
                		 * which has different offsets to the layers, so -:
                		 * # adjust the bounds/offsets so that the rendering is 
                		 *   relative to the correct location.
                		 * # clear the rastports layer pointer so the hook isnt confused */

                		D(bug("[Layers] %s:   Hidden\n", __func__));

                		bounds.MinX -= CR->bounds.MinX;
                                bounds.MinX += ALIGN_OFFSET(CR->bounds.MinX);
                		bounds.MinY -= CR->bounds.MinY;
                		bounds.MaxX -= CR->bounds.MinX;
                                bounds.MaxX += ALIGN_OFFSET(CR->bounds.MinX);
                		bounds.MaxY -= CR->bounds.MinY;

                		rport->BitMap = CR->BitMap;
			    }
        		    _CallLayerHook(hook, rport, rport->Layer, &bounds, offsetX, offsetY, LayersBase);
        		    rport->BitMap = bm;

			} /* hidden cliprect */
        		else
			{
               	           _CallLayerHook(hook, rport, rport->Layer, &bounds, offsetX, offsetY, LayersBase);
			} /* visible cliprect */
        	    } /* if (cliprect intersects rect in screen coords) */
        	} /* ignore hidden simple refresh cliprects */

        	/* Restore the RastPort's Layer, incase it was cleared
        	 * in a rendering operation, but not restored, or we have rendered
        	 * a hidden layer */

        	rport->Layer = L;

        	CR = CR->Next;

	    } /* foreach cliprect */
	} /* if (rect in screen coords interesects layer coords) */

	UnlockLayer(L);
    } /* if (layered rastport) */

    AROS_LIBFUNC_EXIT
} /* DoHookClipRects */
