/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetGBuffers()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(BOOL, GetGBuffers,

/*  SYNOPSIS */
	AROS_LHA(struct AnimOb *, anOb, A0),
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(BOOL , db, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 28, Graphics)

/*  FUNCTION
	Allocate all buffers for a whole AnimOb. In particular this
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
	TRUE, if all the memory allocations were successful, otherwise
	FALSE

    NOTES
	If an AnimOb is passed to GetGBuffers twice new buffers will
	be allocated and therefore old pointers to buffers will be
	lost in space.

    EXAMPLE

    BUGS

    SEE ALSO
	FreeGBuffers() graphics/gels.h

    INTERNALS
	Are real VSprites possible as a part of an AnimOb?
	If yes, then different sizes of memory would have to be
	allocated for BorderLine and CollMask. Currently the
	sizes of memory allocated for this are most of the time
	too large as they are just allocated for a Bob. If this
	code is changed then the code of FreeGBuffers() will
	have to be changed, too, and this text can be erased :-))

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct AnimComp * CurAnimComp = anOb -> HeadComp;

  /* visit all the components of this AnimOb */
  while (NULL != CurAnimComp)
  {
    /* visit all the sequences of a component
       the sequences are connected like a ring!! */
    struct AnimComp * CurSeqAnimComp = CurAnimComp;
    do
    {
      struct Bob * CurBob = CurSeqAnimComp -> AnimBob;
      struct VSprite * CurVSprite = CurBob -> BobVSprite;
      long memsize;

      /* Attention: width of a Bob/VSprite is the number of *words* it
         uses for it's width */


      /* allocate height*(width*2) bytes of Chip-Ram for the ImageShadow */
      memsize =  (CurVSprite -> Height) *
                 (CurVSprite -> Width) * 2;
      if (NULL ==(CurBob -> ImageShadow = AllocMem(memsize, MEMF_CHIP|MEMF_CLEAR)))
        return FALSE;

      /* CollMask points to the same memory as ImageShadow */
      CurVSprite -> CollMask = CurBob -> ImageShadow;

      /* allocate height*(width*2)*depth bytes of Chip-Ram for
         the SaveBuffer */
      memsize *= (CurVSprite -> Depth);
      if (NULL ==(CurBob -> SaveBuffer =  AllocMem(memsize, MEMF_CHIP|MEMF_CLEAR)))
      	return FALSE;


      /* allocate width*2 bytes = width words for BorderLine
       * !!! this is more than enough for VSprites as for a real VSprite
       * its size in pixels is given in CurVSprite->Width
       */
      if (NULL ==(CurVSprite -> BorderLine = AllocMem(CurVSprite -> Width * 2,
                                                      MEMF_CHIP|MEMF_CLEAR)))
        return FALSE;

      /* are we using double buffering for this AnimOb? */
      if (TRUE == db)
      {
        /* allocate a DBufPacket structure */
        if (NULL ==(CurBob -> DBuffer = AllocMem(sizeof(struct DBufPacket),
                                                 MEMF_CLEAR)))
          return FALSE;

        /* BufBuffer needs as much memory as SaveBuffer */
        /* memsize still contains the size of memory required for SaveBuffer */
        if (NULL ==(CurBob -> DBuffer -> BufBuffer =
                    AllocMem(memsize, MEMF_CHIP|MEMF_CLEAR)))
          return FALSE;
      }

      /* go to the next sequence of this component */
      CurSeqAnimComp = CurSeqAnimComp -> NextSeq;
    }
    while (CurAnimComp != CurSeqAnimComp);

    /* go to next component */
    CurAnimComp = CurAnimComp -> NextComp;
  }
  /* all allocations went OK */
  return TRUE;

  AROS_LIBFUNC_EXIT
} /* GetGBuffers */
