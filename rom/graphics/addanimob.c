/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AddAnimOb()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(void, AddAnimOb,

/*  SYNOPSIS */
	AROS_LHA(struct AnimOb *, anOb, A0),
	AROS_LHA(struct AnimOb ** , anKey, A1),
	AROS_LHA(struct RastPort *, rp, A2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 26, Graphics)

/*  FUNCTION
	Link the AnimOb into the list pointed to by AnimKey.
        Calls AddBob with all components of a Bob and initializes
        all the timers of the components of this AnimOb.
	You have to provide a valid GelsInfo structure that is linked
	to the RastPort (InitGels())

    INPUTS
        anOb  = pointer to AnimOb structure to be added to list of
		AnimObs
	anKey = address of a pointer to the first AnimOb in the list
                (when first calling this function the content of
		this address has to be NULL!)
	rp    = pointer to a valid RastPort with initialized GelsInfo
		structure

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels() Animate() graphics/rastport.h graphics/gels.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct AnimComp * CurAnimComp;

  /* this AnimOb becomes the first on in the list*/
  if (NULL != *anKey)
  {
    anOb -> NextOb = (*anKey);
    anOb -> PrevOb = NULL;
    (*anKey) -> PrevOb = anOb;
  }
  *anKey = anOb;

  CurAnimComp = anOb -> HeadComp;

  while (NULL != CurAnimComp)
  {
    /* initialize the timer of each component's first sequence */
    CurAnimComp -> Timer = CurAnimComp -> TimeSet;
    AddBob(CurAnimComp -> AnimBob, rp);
    /* visit the next component */
    CurAnimComp = CurAnimComp -> NextComp;
  }

  AROS_LIBFUNC_EXIT
} /* AddAnimOb */
