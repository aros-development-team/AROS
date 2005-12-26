/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function InitView()
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include <graphics/view.h>
#include <proto/exec.h>

static const struct View defaultView =
{
  NULL, /* ViewPort */
  NULL, /* LOFCprList */
  NULL, /* SHFCprList */
  0,    /* DyOffset */
  0,    /* DxOffset */
  0     /* Modes */
};

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, InitView,

/*  SYNOPSIS */
	AROS_LHA(struct View *, view, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 60, Graphics)

/*  FUNCTION
	Initializes a View structure.

    INPUTS
	view - The View to initialize.

    RESULT
        View is initialized to it`s default values - doesn't care about
        previous contents of this structure.
        All values except for DxOffset,DyOffset are set to 0's.

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

    ASSERT_VALID_PTR(view);
    
    CopyMem ((UBYTE *)&defaultView, view, sizeof (struct View));

    AROS_LIBFUNC_EXIT
    
} /* InitView */
