
/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Draw the list of gels
    Lang: english
*/
#include "graphics_intern.h"

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

    struct VSprite * CurVSprite = rp->GelsInfo->gelHead->NextVSprite;
    struct BitMap bm;
    ULONG i;

    while (NULL != CurVSprite->NextVSprite)
    {
	UBYTE * imagedata = (UBYTE *)CurVSprite->ImageData;
	/*
	 * If this VSprite was overlapping with other VSprites
	 * the follow the ClearPath first and clear all the
	 * VSprites on that Path. That routine will also reset
	 * the ClearPath variable on all encountered VSprites.
	 * If it was not overlapping with other VSprites but
	 * was visible before it will either clear or back up
	 * the previous background.
	 */
void _ClearBobAndFollowClearPath(struct VSprite *, struct RastPort *);

	if (NULL != CurVSprite->VSBob)
	  _ClearBobAndFollowClearPath(CurVSprite,rp);


	InitBitMap(&bm, 
	           CurVSprite->Depth,
		   CurVSprite->Width * 16,
		   CurVSprite->Height);

        /*
         * If I am supposed to back up the background then
         * I have to do it now!!
         */
        if (0 != (CurVSprite->Flags & SAVEBACK) &&
            NULL != CurVSprite->VSBob)
        {  
          UBYTE * savedata = (UBYTE *)CurVSprite->VSBob->SaveBuffer;
	  i=0;
	  while (i < bm.Depth && i < 8)
	  {
	    bm.Planes[i++] = savedata;
	    savedata += bm.Rows * bm.BytesPerRow;
	  }

#if 0
          BltRastPortBitMap(rp,
                            CurVSprite->X,
                            CurVSprite->Y,
                            &bm,
                            0,
                            0,
                            CurVSprite->Width,
                            CurVSprite->Height,
                            0x0c0);
#else
#warning Since BltRastPortBitMap (or something similar) does not exist I need to use ClipBlit here which is pretty ugly.
          {
            struct RastPort rp_bm;
            InitRastPort(&rp_bm);
            rp_bm.BitMap = &bm;
            ClipBlit(rp,
                     CurVSprite->X,
                     CurVSprite->Y,
                     &rp_bm,
                     0,
                     0,
                     CurVSprite->Width,
                     CurVSprite->Height,
                     0x0c0);
          }
#endif
        }


        if (0 == (CurVSprite->Flags & VSPRITE) &&
            0 != (CurVSprite->VSBob->Flags & BOBSAWAY))
        {
          /*
           * This Bob is to be removed...
           */
          CurVSprite->PrevVSprite->NextVSprite = CurVSprite->NextVSprite;
          CurVSprite->NextVSprite->PrevVSprite = CurVSprite->PrevVSprite;
#warning This damages the drawpath and clearpath!
        }
        else
        {
  	  /*
	   * Now draw the VSprite/Bob at its current location.
	   * The bitmap has already been initialized!
	   */
	  i =0;
	  while (i < bm.Depth && i < 8)
	  {
	    bm.Planes[i++] = imagedata;
	    imagedata += bm.Rows * bm.BytesPerRow;
	  }

	  BltBitMapRastPort(&bm,
                  	    0,
                	    0,
                	    rp,
                	    CurVSprite->X,
                	    CurVSprite->Y,
                	    CurVSprite->Width *16,
                	    CurVSprite->Height,
                	    0x0c0);

	  /*
	   * I will need to know the vsprite's coordinates
	   * that it has now the next time as well for clearing
	   * purposes.
	   */
          CurVSprite->OldX = CurVSprite->X;
          CurVSprite->OldY = CurVSprite->Y;
        
          if (0 == (CurVSprite->Flags & VSPRITE))
          {
            /*
             * it's a bob! mark it as drawn.
             */
            CurVSprite->VSBob->Flags |= BDRAWN;
          }
        }
	CurVSprite = CurVSprite->NextVSprite;
    }

    AROS_LIBFUNC_EXIT
} /* DrawGList */


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
                                 struct RastPort * rp)
{
  if (NULL != CurVSprite->ClearPath)
  {
    /*
     * Clear the next one first. (recursion!!!)
     */
    _ClearBobAndFollowClearPath(CurVSprite->ClearPath, rp);
    CurVSprite->ClearPath = NULL;
  }


  if (0 != (CurVSprite->Flags & BACKSAVED))
  { 
    ULONG i;
    struct BitMap bm;
    UBYTE * imagedata = (UBYTE *)CurVSprite->VSBob->SaveBuffer;
    
    /*
     * Restore background!
     */	 
    InitBitMap(&bm, 
               CurVSprite->Depth,
               CurVSprite->Width * 16,
               CurVSprite->Height);
    i=0;
    while (i < bm.Depth && i < 8)
    {
      bm.Planes[i++] = imagedata;
      imagedata += bm.Rows * bm.BytesPerRow;
    }

#if 0
    BltBitMapRastPort(&bm,
                      0,
                      0,
                      rp,
                      CurVSprite->OldX,
                      CurVSprite->OldY,
                      CurVSprite->Width *16,
                      CurVSprite->Height,
                      0x0c0);
#endif

    CurVSprite->Flags &= ~BACKSAVED;
  }
  else
  {
    /*
     * No background was saved. So let's restore the
     * standard background!
     */

      EraseRect(rp,
                CurVSprite->OldX,
                CurVSprite->OldY,
                CurVSprite->OldX + ( CurVSprite->Width << 4 ) - 1,
                CurVSprite->OldY +   CurVSprite->Height       - 1);
  }
}
