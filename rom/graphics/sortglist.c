/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics function SortGList()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, SortGList,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 25, Graphics)

/*  FUNCTION
	Sort the current gel list by the y and x coordinates of it's
        elements.
        You have to call this routine prior to calling DoCollision()
        of DrawGList or make sure that the list is sorted!

    INPUTS
	rp  = pointer to RastPort that has an GelsInfo linked to it

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels() DrawGList() DoCollision() graphics/rastport.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct VSprite * CorrectVSprite = rp -> GelsInfo -> gelHead;
  struct VSprite * CurVSprite = CorrectVSprite -> NextVSprite;

  long Coord = (CorrectVSprite -> Y << 16) + CorrectVSprite -> X;
  long NewCoord =  (CurVSprite -> Y << 16) + CurVSprite     -> X;

  while ( NULL != CurVSprite )
  {
    if (Coord <= NewCoord)
    {
      /* sorting is not necessary for this VSprite */
      CorrectVSprite = CurVSprite;
      CurVSprite     = CurVSprite -> NextVSprite;

      Coord    = NewCoord;
      NewCoord =  (CurVSprite -> Y << 16) + CurVSprite -> X;
    }
    else
    {
      struct VSprite * tmpVSprite = CorrectVSprite -> PrevVSprite;
      /* the CurVSprite is on the wrong position */

      /* unlink it from it's current position */
      CorrectVSprite -> NextVSprite = CurVSprite -> NextVSprite;
      CurVSprite -> NextVSprite -> PrevVSprite = CorrectVSprite;

      /* insert CurVSprite at some previous place */
      while ( (tmpVSprite -> Y << 16) + tmpVSprite -> X > NewCoord)
        tmpVSprite = tmpVSprite -> PrevVSprite;

      /* link CurVSprite *after* tmpVSprite into the list */
      CurVSprite -> NextVSprite = tmpVSprite -> NextVSprite;
      CurVSprite -> PrevVSprite = tmpVSprite;
      tmpVSprite -> NextVSprite = CurVSprite;
      CurVSprite -> NextVSprite -> PrevVSprite = CurVSprite;

      /* set the new CurVSprite and read it's coordinate */
      CurVSprite = CorrectVSprite -> NextVSprite;
      NewCoord =  (CurVSprite -> Y << 16) + CurVSprite -> X;
    }
  }

  AROS_LIBFUNC_EXIT
} /* SortGList */
