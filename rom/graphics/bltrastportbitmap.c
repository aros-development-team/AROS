/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Blit the content of a rastport into a bitmap
    Lang: english
*/
#include <aros/debug.h>
#include <proto/graphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH9(void, BltRastPortBitMap,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, srcRastPort, A0),
	AROS_LHA(LONG             , xSrc       , D0),
	AROS_LHA(LONG             , ySrc       , D1),
	AROS_LHA(struct BitMap *  , destBitMap , A1),
	AROS_LHA(LONG             , xDest      , D2),
	AROS_LHA(LONG             , yDest      , D3),
	AROS_LHA(ULONG            , xSize      , D4),
	AROS_LHA(ULONG            , ySize      , D5),
	AROS_LHA(ULONG            , minterm    , D6),

/*  LOCATION */
	struct GfxBase *, GfxBase, 196, Graphics)

/*  FUNCTION

        Copies the content of the rast port into the bitmap.
        Takes cliprects into consideration.
    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
	struct Layer * srcLayer;

    	FIX_GFXCOORD(xSrc);
    	FIX_GFXCOORD(ySrc);
    	FIX_GFXCOORD(xDest);
    	FIX_GFXCOORD(yDest);
	
	if (NULL == (srcLayer = srcRastPort->Layer)) {
		/*
		 * Rastport without a layer is a screen.
		 */
		BltBitMap(srcRastPort->BitMap,
		          xSrc,
		          ySrc,
		          destBitMap,
		          xDest,
		          yDest,
		          xSize,
		          ySize,
		          minterm,
		          ~0,
		          NULL);
	} else {
		struct BitMap * srcBM = srcRastPort->BitMap;
		struct ClipRect * srcCR;
		int area = xSize * ySize;
		UBYTE useminterm = 0;
		ULONG bltMask = 0xFFFFFFFF;

		LockLayerRom(srcLayer);
		srcCR = srcLayer->ClipRect;
		
		while (NULL != srcCR &&
		       area > 0) {
			/*
			 * Is the current cliprect withing the
			 * required region?
			 */
			LONG crX0, crX1, crY0, crY1;
			/* cr?? have to be coordinates related to the rastport */
			crX0 = srcCR->bounds.MinX - srcLayer->bounds.MinX;
			crX1 = srcCR->bounds.MaxX - srcLayer->bounds.MinX;
			crY0 = srcCR->bounds.MinY - srcLayer->bounds.MinY;
			crY1 = srcCR->bounds.MaxY - srcLayer->bounds.MinY;

			/* the only case that must not happen is that
			  this ClipRect is outside the source area.  */

			if (!(crX0 > (xSrc+xSize-1) ||
			      crX1 <  xSrc          || 
			      crY0 > (ySrc+ySize-1) ||
			      crY1 <  ySrc)) {
				ULONG MinX, MinY;
				ULONG bltSrcX, bltSrcY, bltDestX, bltDestY, bltWidth, bltHeight;
				ULONG SrcOffsetX;

				/* this cliprect contains bitmap data that need to be copied */
				/* 
				 * get the pointer to the bitmap structure and fill out
				 * the rectangle structure that shows which part we mean to copy		
				 */
				if (NULL != srcCR->BitMap) {
					if (0 == (srcLayer->Flags & LAYERSUPER)) {
						/* no superbitmap */
						SrcOffsetX = ALIGN_OFFSET(srcCR->bounds.MinX);

						if (xSrc >= crX0) {
							bltSrcX  = xSrc - crX0 + SrcOffsetX;
							bltDestX = 0;
						} else {
							bltSrcX  = SrcOffsetX;
							bltDestX = crX0 - xSrc;
						}

						if (ySrc > crY0) {
							bltSrcY	 = ySrc - crY0;
							bltDestY = 0;
						} else {
							bltSrcY	 = 0;
							bltDestY = crY0 - ySrc;
						}
						
						srcBM = srcCR->BitMap;
					} else {
						/* with superbitmap */
						if (xSrc >= crX0) {
							bltSrcX  = xSrc - srcLayer->Scroll_X;
							bltDestX = 0;
						} else {
							bltSrcX  = crX0 - srcLayer->Scroll_X;
							bltDestX = crX0 - xSrc;
						}
						
						if (ySrc >= crY0) {
							bltSrcY	 = ySrc - srcLayer->Scroll_Y;
							bltDestX = 0;
						} else {
							bltSrcY	 = crY0 - srcLayer->Scroll_Y;
							bltDestY = crY0 - ySrc;
						}

						srcBM = srcCR->BitMap;
					}

				} else {
					/* this part of the layer is not hidden. */
					/* The source bitmap is the bitmap of the rastport */
					srcBM	 = srcRastPort->BitMap;
						
					/* xSrc and ySrc are relative to the rastport of the window
					 * or layer - here we have to make them absolute to the
					 * screen's rastport
					 */
							
					if (xSrc <= crX0) {
						bltSrcX  = srcCR->bounds.MinX;
						bltDestX = crX0 - xSrc;
					} else {
						bltSrcX  = xSrc + srcLayer->bounds.MinX;
						bltDestX = 0;
					}
								
					if (ySrc <= crY0) {
						bltSrcY  = srcCR->bounds.MinY;
						bltDestY = crY0 - ySrc;
					} else {
						bltSrcY  = ySrc + srcLayer->bounds.MinY;
						bltDestY = 0;
					}
				}

				if (crX0 > xSrc)
					MinX = crX0 - xSrc;
				else
					MinX = 0;

				if (crX1 < (xSrc+xSize-1)) 
					bltWidth = crX1 - xSrc - MinX + 1;
				else
					bltWidth = xSize - 1 - MinX + 1;

				if (crY0 > ySrc)
					MinY = crY0 - ySrc;
				else
					MinY = 0;

				if (crY1 < (ySrc+ySize-1))
					bltHeight = crY1 - ySrc - MinY + 1;
				else
					bltHeight = ySize - 1 - MinY + 1;
		
				if ((0 != (srcLayer->Flags & LAYERSIMPLE) &&
				    (NULL != srcCR->lobs ||
					0 != (srcCR->Flags & CR_NEEDS_NO_CONCEALED_RASTERS))))
					useminterm = 0x0;		 /* clear this area in the destination */
				else
					useminterm = minterm;
				
			
				/*
				 * Finally blit from the srcBM to the
				 * destBM
				 */
				BltBitMap(srcBM,
				          bltSrcX,
				          bltSrcY,
				          destBitMap,
				          bltDestX,
				          bltDestY,
				          bltWidth,
				          bltHeight,
				          useminterm,
				          bltMask,
				          NULL);
				area -= (bltWidth * bltHeight);
			}
			srcCR = srcCR->Next;	
		}
		UnlockLayerRom(srcLayer);
	}
	ReturnVoid("BltRastPortBitMap");

	AROS_LIBFUNC_EXIT
} /* BltBitMapRastPort */
