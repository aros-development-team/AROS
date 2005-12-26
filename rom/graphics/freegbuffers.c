/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FreeGBuffers()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(void, FreeGBuffers,

/*  SYNOPSIS */
	AROS_LHA(struct AnimOb *, anOb, A0),
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(BOOL , db, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 100, Graphics)

/*  FUNCTION
	DeAllocate all buffers for a whole AnimOb. In particular this
	means getting buffers for
	- BorderLine
	- SaveBuffer
	- CollMask
	- ImageShadow (points to the same memory as CollMask does)
	- if db is set to TRUE the user wants double-buffering, so we need
	  - DBufPacket
	  - BufBuffer

    INPUTS
        anOb = pointer to AnimOb structure to be added to list of
	       AnimObs
	rp   = pointer to a valid RastPort with initialized GelsInfo
	       structure
	db   = TRUE when double-buffering is wanted

    RESULT

    NOTES
	A call to GetGBuffers() that resulted in a partially allocation
	of the required buffers will result in a deallocation of these
	buffers. (Possible incompatibility with the real thing, though)

    EXAMPLE

    BUGS

    SEE ALSO
	GetGBuffers() graphics/rastport.h graphics/gels.h

    INTERNALS
	See FreeGBuffers() !!

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct AnimComp * CurAnimComp = anOb -> HeadComp;

  /* visit all the components of this AnimOb */
  while (NULL != CurAnimComp)
  {
    struct AnimComp * CurSeqAnimComp = CurAnimComp;
    /* visit all the sequences of a component
       the sequences are connected like a ring!! */
    do
    {
      struct Bob * CurBob = CurSeqAnimComp -> AnimBob;
      struct VSprite * CurVSprite = CurBob -> BobVSprite;
      long memsize;

      /* Attention: width of a Bob/VSprite is the number of *words* it
         uses for it's width */

      /* deallocate height*(width*2) bytes of Chip-Ram for the ImageShadow */
      memsize =  (CurVSprite -> Height) *
                 (CurVSprite -> Width) * 2;
      if (NULL != CurBob -> ImageShadow)
        FreeMem(CurBob -> ImageShadow, memsize);

      /* CollMask could point to the same memory as ImageShadow but
         is not necessarly the same */
      if (CurBob -> ImageShadow != CurVSprite -> CollMask)
        FreeMem(CurVSprite -> CollMask, memsize);

      CurBob -> ImageShadow  = NULL;
      CurVSprite -> CollMask = NULL;

      /* deallocate height*(width*2)*depth bytes of Chip-Ram for
         the SaveBuffer */
      memsize *= (CurVSprite -> Depth);
      if (NULL != CurBob -> SaveBuffer)
      {
        FreeMem(CurBob -> SaveBuffer, memsize);
        CurBob -> SaveBuffer = NULL;
      }


      /* deallocate width bytes for BorderLine */
      if (NULL != CurVSprite -> BorderLine)
      {
        FreeMem(CurVSprite -> BorderLine, CurVSprite -> Width * 2);
        CurVSprite -> BorderLine = NULL;
      }

      /* were we using double buffering for this AnimOb? */
      if (TRUE == db && NULL != CurBob -> DBuffer)
      {
        /* BufBuffer needed as much memory as SaveBuffer */
        /* memsize still contains the size of memory required for SaveBuffer */
        if (NULL != CurBob -> DBuffer -> BufBuffer)
          FreeMem(CurBob -> DBuffer -> BufBuffer, memsize);

        /* deallocate the DBufPacket structure */
        FreeMem(CurBob -> DBuffer, sizeof(struct DBufPacket));
        CurBob -> DBuffer = NULL;
      }

      /* go to the next sequence of this component */
      CurSeqAnimComp = CurSeqAnimComp -> NextSeq;
    }
    while (CurAnimComp != CurSeqAnimComp);

    /* go to next component */
    CurAnimComp = CurAnimComp -> NextComp;
  }

  AROS_LIBFUNC_EXIT
} /* FreeGBuffers */
