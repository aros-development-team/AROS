/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function InitMasks()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, InitGMasks,

/*  SYNOPSIS */
	AROS_LHA(struct AnimOb *, anOb, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 29, Graphics)

/*  FUNCTION
	For every component's sequence initilize the Masks by calling
	InitMasks()

    INPUTS
        anOb = pointer to the AnimOb

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels() InitMasks() graphics/gels.h

    INTERNALS

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
    /* visit all sequences of the current component of this AnimOb
     * they might be connected like a ring (most probably are)!
     */
    do
    {
      InitMasks(CurSeqAnimComp -> AnimBob -> BobVSprite);

      /* go to the next sequence of this component */
      CurSeqAnimComp = CurSeqAnimComp -> NextSeq;
    }
    while (CurAnimComp != CurSeqAnimComp && NULL != CurAnimComp );

    /* go to next component */
    CurAnimComp = CurAnimComp -> NextComp;
  }

  AROS_LIBFUNC_EXIT
} /* InitGMasks */
