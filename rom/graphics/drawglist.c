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

  struct VSprite * CurVSprite = rp->GelsInfo->gelHead;
  struct BitMap * bm = AllocMem(sizeof(struct BitMap), MEMF_ANY);

  bm->Flags       = 0;
  
  while (NULL != CurVSprite)
  {
    ULONG i = 0;
    UBYTE * imagedata = (UBYTE *)CurVSprite->ImageData;
    
    bm->BytesPerRow = CurVSprite->Width << 1;
    bm->Rows        = CurVSprite->Height;
    bm->Depth       = CurVSprite->Depth;
    
    while (i < bm->Depth && i < 8)
    {
      bm->Planes[i++] = imagedata;
      imagedata += bm->Rows * bm->BytesPerRow;
    }
    
    BltBitMapRastPort(bm,
                      0,
                      0,
                      rp,
                      CurVSprite->X,
                      CurVSprite->Y,
                      CurVSprite->Width  << 4,
                      CurVSprite->Height,
                      0x0c0);
    
    CurVSprite = CurVSprite->NextVSprite;
  }

  FreeMem(bm, sizeof(struct BitMap));

  AROS_LIBFUNC_EXIT
} /* DrawGList */


#if 0
  Some routines to cut and paste later on...
  
  /* If there's data stored in the savebuffer then restore it */
  if (0 != (CurVSprite->Flags & BACKSAVED))
  {
    imagedate = CurVSprite->VSBob->SaveBuffer;
    bm->BytesPerRow = CurVSprite->Width << 1;
    bm->Rows        = CurVSprite->Height;
    bm->Depth       = CurVSprite->Depth;
    
    while (i < bm->Depth && i < 8)
    {
      bm->Planes[i++] = imagedata;
      imagedata += bm->Rows * bm->BytesPerRow;
    }
   
    BltBitMapRastPort(bm,
                      0,
                      0,
                      CurVSprite->OldX,
                      CurVSprite->OldY,
                      CurVSprite->Width  << 4,
                      CurVSprite->Height,
                      0x0c0);
  }
#endif