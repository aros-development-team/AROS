/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Internal function for improved gels handling.
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include <exec/memory.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include "gels_internal.h"

struct IntVSprite * _CreateIntVSprite(struct VSprite * vs, struct RastPort * rp)
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
			_ValidateIntVSprite(ivs,rp, FALSE);
	}
	return ivs;
}


VOID _DeleteIntVSprite(struct VSprite * vs)
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
                         BOOL force_change)
{
	struct VSprite * vs = ivs->VSprite;
	/*
	 * Check whether the ImageData pointer has changed
	 */
	if (vs->ImageData != ivs->OrigImageData ||
	    TRUE == force_change) {
		struct BitMap bm;

#if 1
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
			                             0,
			                             rp->BitMap); //!!!

			ivs->SaveBuffer = AllocBitMap(vs->Width<<4,
			                              vs->Height,
			                              vs->Depth,
			                              0,
			                              rp->BitMap); //!!!
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
		          ivs->Width*16,
		          ivs->Height,
		          0x0c0,
		          vs->PlanePick,
		          NULL);
			  
	}
	
	return TRUE;
}
