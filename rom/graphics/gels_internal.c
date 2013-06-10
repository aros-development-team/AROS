/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal function for improved gels handling.
    Lang: english
*/
#include <aros/debug.h>

#include <graphics/gels.h>
#include <graphics/rastport.h>
#include <exec/memory.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include "gels_internal.h"
#include "graphics_intern.h"

struct IntVSprite * _CreateIntVSprite(struct VSprite * vs, 
                                      struct RastPort * rp,
                                      struct GfxBase * GfxBase)
{
	struct IntVSprite * ivs = AllocMem(sizeof(struct IntVSprite),
	                                   MEMF_CLEAR);
	if (NULL != ivs) {
		ivs -> VSprite = vs;
		vs->IntVSprite = ivs;

		/* 
		 * Don't call the validate function when rp=NULL, i.e. when 
		 * called by InitGels().
		 */
		if (NULL != rp)
			_ValidateIntVSprite(ivs,
			                    rp, 
			                    FALSE,
			                    GfxBase);
	}
	return ivs;
}


VOID _DeleteIntVSprite(struct VSprite * vs,
                       struct GfxBase * GfxBase)
{
	struct IntVSprite * ivs = vs->IntVSprite;

	if (NULL != ivs) {
		if (NULL != ivs->ImageData)
			FreeBitMap(ivs->ImageData);

		if (NULL != ivs->SaveBuffer)
			FreeBitMap(ivs->SaveBuffer);
		
		FreeMem(ivs, sizeof(struct IntVSprite));
		vs->IntVSprite = NULL;
	}
}


/*
 * Check whether the appearance of the VSprite has been changed
 * somehow. If for example the Image Data has changed, then
 * I will try to update the BitMap of the IntVSprite structure
 * to the new image data.
 */
BOOL _ValidateIntVSprite(struct IntVSprite * ivs, 
                         struct RastPort * rp,
                         BOOL force_change,
                         struct GfxBase * GfxBase)
{
	struct VSprite * vs = ivs->VSprite;
	/*
	 * Check whether the ImageData pointer has changed
	 */
	if (vs->ImageData != ivs->OrigImageData ||
	    force_change) {
		struct BitMap bm;

#if 0
kprintf("%s: Imagedata has changed (old:%p-new:%p)!\n",
        __FUNCTION__,
        vs->ImageData,
        ivs->OrigImageData);
kprintf("PlanePick: %02x, rp->BitMap:%p\n",vs->PlanePick,rp->BitMap);
#endif
		/*
		 * Only need to get a new bitmap if
		 * something in the size of the bob has changed.
		 */
		if ((ivs->Width  != vs->Width )  ||
		    (ivs->Height != vs->Height)  ||
		    (ivs->Depth  != vs->Depth )    ) {
			if (NULL != ivs->ImageData)
				FreeBitMap(ivs->ImageData);

			if (NULL != ivs->SaveBuffer)
				FreeBitMap(ivs->SaveBuffer);
			/*
			 * Now get a new bitmap
			 */

			ivs->ImageData = AllocBitMap(vs->Width<<4,
			                             vs->Height,
			                             vs->Depth,
			                             BMF_CLEAR,
			                             rp->BitMap);

			ivs->SaveBuffer = AllocBitMap(vs->Width<<4,
			                              vs->Height,
			                              vs->Depth,
			                              0,
			                              rp->BitMap);
			ivs->Width  = vs->Width;
			ivs->Height = vs->Height;
			ivs->Depth  = vs->Depth;
		}

		ivs->OrigImageData = vs->ImageData;

		/*
		 * Blit the image data from the VSprite into the
		 * ImageData (BitMap) of the IntVSprite
		 */
		InitBitMap(&bm,
		           ivs->Depth,
		           ivs->Width<<4,
		           ivs->Height);

    	    	{
		    UBYTE *imagedata = (UBYTE *)vs->ImageData;
		    WORD  d, shift;
		    
		    for (d = 0, shift = 1; d < 8; d++, shift *= 2)
		    {
		    	if (vs->PlanePick & shift)
			{
			    bm.Planes[d] = imagedata;
			    imagedata += (bm.Rows * bm.BytesPerRow);
			}
			else
			{
			    bm.Planes[d] = (vs->PlaneOnOff & shift) ? (PLANEPTR)-1 : NULL;
			}
		    }
		    
		}
		
		BltBitMap(&bm,
		          0,
		          0,
		          ivs->ImageData,
		          0,
		          0,
		          ivs->Width << 4,
		          ivs->Height,
		          0x0c0,
		          vs->PlanePick,
		          NULL);
			  
	}
	
	return TRUE;
}

/*
 * Erase the VSprite from the list and follow the clear path
 * first, if necessary! This way of organizing the ClearPath
 * makes it easy to implement RemIBob but it leads to a recursion!
 * RemIBob could simply call this function here and then redraw
 * all cleared Bobs.
 * If a recursion is not what we want this can be easily
 * changed.
 */
 
void _ClearBobAndFollowClearPath(struct VSprite * CurVSprite, 
                                 struct RastPort * rp,
                                 struct GfxBase * GfxBase)
{
	/*
	 * If the bob has not been drawn, yet, then don't do anything.
	 * If the bob has already been cleared, then also leave here!
	 * It does happen that this routine gets called for 
	 * a sprite that has been cleared already.
	 */
	if (0 != (CurVSprite->VSBob->Flags & (BWAITING|BOBNIX))) {
		CurVSprite->VSBob->Flags &= ~BWAITING;
		return;
	}

	if (NULL != CurVSprite->ClearPath) {
		/*
		 * Clear the next one first. (recursion!!!)
		 */
		_ClearBobAndFollowClearPath(CurVSprite->ClearPath, 
		                            rp,
		                            GfxBase);
		CurVSprite->ClearPath = NULL;
	}


	/*
	 * Only restore the background if the bob image
	 * that is currently there is to be replaced by
	 * the background. If SAVEBOB is set the user
	 * might want some kind of a brush effect.
	 */
	
	if (0 == (CurVSprite->Flags & SAVEBOB)) {
		if (0 != (CurVSprite->Flags & BACKSAVED)) {
			BltBitMapRastPort(CurVSprite->IntVSprite->SaveBuffer,
			                  0,
			                  0,
			                  rp,
			                  CurVSprite->OldX,
			                  CurVSprite->OldY,
			                  CurVSprite->Width << 4,
			                  CurVSprite->Height,
			                  0x0c0);
			CurVSprite->Flags &= ~BACKSAVED;
			
		} /* if (0 != (CurVSprite->Flags & BACKSAVED)) */
		  else {
			/*
			 * No background was saved. So let's restore the
			 * standard background!
			 */
			EraseRect(rp,
			          CurVSprite->OldX,
			          CurVSprite->OldY,
			          CurVSprite->OldX + ( CurVSprite->Width << 4 ) - 1,
			          CurVSprite->OldY +   CurVSprite->Height	- 1);
		}
		/*
		 * Mark the BOB as cleared.
		 */
		CurVSprite->VSBob->Flags |= BOBNIX;
	} /* if (0 == (CurVSprite->Flags & SAVEBOB)) */
}
