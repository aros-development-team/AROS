/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function AreaEllipse()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH5(ULONG, AreaEllipse,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(WORD             , cx, D0),
	AROS_LHA(WORD             , cy, D1),
	AROS_LHA(WORD             , a , D2),
	AROS_LHA(WORD             , b , D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 31, Graphics)

/*  FUNCTION
	Add an ellipse to the vector buffer. An ellipse takes up two
	entries in the buffer.

    INPUTS
	rp - pointer to a valid RastPort structure with a pointer to
	     the previously initilized AreaInfo structure.
	cx - x coordinate of the centerpoint relative to rastport
	cy - y coordinate of the centerpoint relative to rastport
	a  - horizontal radius of the ellipse (> 0)
	b  - vertical radius of the ellipse (> 0)

    RESULT
	error -  0 for success
	        -1 if the vector collection matrix is full

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitArea() AreaMove() AreaDraw() AreaCircle() graphics/rastport.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct AreaInfo * areainfo = rp->AreaInfo;

  /*  Is there still enough storage area in the areainfo-buffer?
   *  We need at least a storage area for two vectors
   */
  if (areainfo->Count + 1 < areainfo->MaxCount)
  {
    /* is this the very first entry in the vector collection matrix */
    if (0 == areainfo->Count)
    {
      areainfo->VctrPtr[0] = cx;
      areainfo->VctrPtr[1] = cy;
      areainfo->FlagPtr[0] = 0x83;

      areainfo->VctrPtr[2] = a;
      areainfo->VctrPtr[3] = b;
      areainfo->FlagPtr[1] = 0x0;

      areainfo->VctrPtr    = &areainfo->VctrPtr[4];
      areainfo->FlagPtr    = &areainfo->FlagPtr[2];

      areainfo->Count += 2;
    }
    else
    {
      /* close the previous polygon if necessary */
      if (0x03 == areainfo->FlagPtr[-1] &&
          (areainfo->VctrPtr[-1] != areainfo->FirstY ||
           areainfo->VctrPtr[-2] != areainfo->FirstX   ) )
      {
        /* there will be enough storage area for this AreaDraw() call:
         * no need to check for errors
         */
        AreaDraw(rp, areainfo->FirstX, areainfo->FirstY);
      }

      /*  If the previous command in the vector collection matrix was a move then
       *  erase that one
       */

      if (0x00 == areainfo->FlagPtr[-1])
      {
        areainfo->VctrPtr = &areainfo->VctrPtr[-2];
        areainfo->FlagPtr--;
        areainfo->Count--;
      }

      /* still enough storage area?? */
      if (areainfo->Count + 1 < areainfo->MaxCount)
      {
        areainfo->VctrPtr[0] = cx;
        areainfo->VctrPtr[1] = cy;
        areainfo->FlagPtr[0] = 0x83;

        areainfo->VctrPtr[2] = a;
        areainfo->VctrPtr[3] = b;
        areainfo->FlagPtr[1] = 0x00;

        areainfo->VctrPtr    = &areainfo->VctrPtr[4];
        areainfo->FlagPtr    = &areainfo->FlagPtr[2];

        areainfo->Count += 2;

        return 0;
      }
      else
        return -1;
    } /* else branch of if (0 == areainfo->Count) */

    /* will never get to this point! */
  }
  
  return -1;  
  AROS_LIBFUNC_EXIT
} /* AreaEllipse */
