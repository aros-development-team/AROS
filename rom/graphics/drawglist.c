/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Draw the list of gels
    Lang: english
*/

#include <aros/debug.h>
#include <proto/graphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "gels_internal.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH2(void, DrawGList,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 19, Graphics)

/*  FUNCTION
	Process the gel list, draw VSprites and Bobs.

    INPUTS
	rp - RastPort where Bobs will be drawn
	vp - ViewPort for VSprites

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

	struct VSprite * CurVSprite = rp->GelsInfo->gelHead;
	struct VSprite * AfterPathVSprite = NULL;
	int              followdrawpath;
	struct VSprite * PrevVSprite = NULL;

	/*
	 * CurVSprite is not a valid VSprite but the "boundary vsprite"
	 * Should I follow the DrawPath?
	 */
	if (NULL != CurVSprite->IntVSprite && 
	    NULL != CurVSprite->IntVSprite->DrawPath) {
		followdrawpath = TRUE;
		AfterPathVSprite = CurVSprite->NextVSprite;
		CurVSprite = CurVSprite->IntVSprite->DrawPath;
	} else {
		followdrawpath = FALSE;
		CurVSprite = CurVSprite->NextVSprite;
	}

	/*
	 * As long as I don't step on the last boundary vsprite
	 */
	while (NULL != CurVSprite->NextVSprite) {
		/*
		 * Check a few flags that must not be set.
		 * The Bob should not be out of the rastport or already be drawn
		 */
		if ( 0 == (CurVSprite->Flags & (GELGONE)) ||
		   (NULL != (CurVSprite->VSBob) && 0 == (CurVSprite->VSBob->Flags & BDRAWN))) {
			/*
			 * If this VSprite was overlapping with other VSprites
			 * the follow the ClearPath first and clear all the
			 * VSprites on that path. That routine will also reset
			 * the ClearPath variable on all encountered VSprites.
			 * If it was not overlapping with other VSprites but
			 * was visible before it will either clear or back up
			 * the previous background.
			 */

			if (NULL != CurVSprite->VSBob)
				_ClearBobAndFollowClearPath(CurVSprite,rp,GfxBase);

			/*
			 * If I am supposed to back up the background then
			 * I have to do it now!!
			 * This unfortunatley has also to be done if the
			 * VSprite/Bob did not move since it could have
			 * changed its appearance.
			 */
			if (0 != (CurVSprite->Flags & SAVEBACK) && NULL != CurVSprite->VSBob)
			{
				BltRastPortBitMap(rp,
		                        	  CurVSprite->X,
		                        	  CurVSprite->Y,
		                        	  CurVSprite->IntVSprite->SaveBuffer,
		                        	  0,
		                        	  0,
		                        	  CurVSprite->Width << 4,
		                        	  CurVSprite->Height,
		                        	  0x0c0, GfxBase);

				CurVSprite->Flags |= BACKSAVED;
			}
			else kprintf("NOT SAVING BACKGROUND!\n");

			if (0 == (CurVSprite->Flags & VSPRITE) &&
			    BOBSAWAY == (CurVSprite->VSBob->Flags & BOBSAWAY)) {
				/*
				 * This Bob is to be removed...
				 */
				CurVSprite->PrevVSprite->NextVSprite = CurVSprite->NextVSprite;
				CurVSprite->NextVSprite->PrevVSprite = CurVSprite->PrevVSprite;
				/*
				 * This does not damage the drawpath and clearpath since
				 * the structure is not freed.
				 */
			} else {
				/*
				 * Now draw the VSprite/Bob at its current location.
				 */
				_ValidateIntVSprite(CurVSprite->IntVSprite, 
				                    rp, 
				                    FALSE,
				                    GfxBase);
				
				/* FIXME: Wrong minterm is used.
				 * Need to implement mintern '0xe0'.
				 */
				BltBitMapRastPort(CurVSprite->IntVSprite->ImageData,
				                  0,
		                  		  0,
				                  rp,
				                  CurVSprite->X,
				                  CurVSprite->Y,
				                  CurVSprite->Width << 4,
				                  CurVSprite->Height,
				                  0x0c0 /* should be 0xe0! */);
				/*
				 * I will need to know the vsprite's coordinates
				 * that it has now the next time as well for clearing
				 * purposes.
				 */
				CurVSprite->OldX = CurVSprite->X;
				CurVSprite->OldY = CurVSprite->Y;

				if (CurVSprite->VSBob) {
					/*
					 * it's a bob! mark it as drawn.
					 */
					CurVSprite->VSBob->Flags |= BDRAWN;
					CurVSprite->VSBob->Flags &= ~BOBNIX;
				}
			}
		}
	
		/*
		 * Am I supposed to follow the drawpath.
		 * If yes then follow it to its end.
		 */
	 
		if (followdrawpath) {
			if (NULL != PrevVSprite)
				PrevVSprite->ClearPath = CurVSprite;
			PrevVSprite = CurVSprite;

			CurVSprite = CurVSprite->IntVSprite->DrawPath;
			if (NULL == CurVSprite) {
				followdrawpath = FALSE;
				PrevVSprite = NULL;
				CurVSprite = AfterPathVSprite;
			}
		} else {	
			if (NULL != PrevVSprite)
				PrevVSprite->ClearPath = CurVSprite;
			PrevVSprite = CurVSprite;
			CurVSprite = CurVSprite->NextVSprite;
			/*
			 * Does a DrawPath start here?
			 * If yes, then I will follow the DrawPath 
			 * after I am done with this VSprite.
			 */
			if (NULL != CurVSprite->IntVSprite->DrawPath) {
				followdrawpath = TRUE;
				AfterPathVSprite = CurVSprite->NextVSprite;
			}
		}
	} /* while not all bobs/vsprites are drawn */

	AROS_LIBFUNC_EXIT
    
} /* DrawGList */
