/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FreeVPortCopLists()
    Lang: english
*/
#include <graphics/copper.h>
#include <graphics/view.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, FreeVPortCopLists,

/*  SYNOPSIS */
	AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 90, Graphics)

/*  FUNCTION
	Deallocate all memory associated with the CopList structures
	for display, color, sprite and the user copper list. The
	corresponding fields in this structure will be set to NULL.

    INPUTS
	vp - pointer to a ViewPort structure

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	graphics/view.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  if (NULL != vp->DspIns)
  {
    FreeCopList(vp->DspIns);
    vp->DspIns = NULL;
  }

  if (NULL != vp->SprIns)
  {
    FreeCopList(vp->SprIns);
    vp->SprIns = NULL;
  }

  if (NULL != vp->ClrIns)
  {
    FreeCopList(vp->ClrIns);
    vp->ClrIns = NULL;
  }

  if (NULL != vp->UCopIns)
  {
    if (NULL != vp->UCopIns->FirstCopList)
      FreeCopList(vp->UCopIns->FirstCopList);

    FreeMem(vp->UCopIns, sizeof(struct UCopList));
    vp->UCopIns = NULL;
  }


  AROS_LIBFUNC_EXIT
} /* FreeVPortCopLists */
