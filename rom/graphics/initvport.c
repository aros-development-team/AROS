/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function InitVPort()
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include <graphics/view.h>
#include <proto/exec.h>

static const struct ViewPort defaultViewPort =
{
  NULL, /* Next */
  NULL, /* ColorMap */
  NULL, /* DspIns */
  NULL, /* SprIns */
  NULL, /* ClrIns */
  NULL, /* UCopIns */
  0,    /* DWidth */
  0,    /* DHeight */
  0,    /* DxOffset */
  0,    /* DyOffset */
  0,    /* Modes */
  0x24, /* SpritePriorities */
  0,    /* ExtendedModes */
  NULL  /* RasInfo */
};

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, InitVPort,

/*  SYNOPSIS */
	AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 34, Graphics)

/*  FUNCTION
	Initializes a ViewPort structure.

    INPUTS
	view - The View to initialize.

    RESULT
        ViewPort is initialized to it`s default values - doesn't care about
        previous contents of this structure.
        All values except for SpritePriorities are set to 0's.

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

    ASSERT_VALID_PTR(vp);
    
    CopyMem ((UBYTE *)&defaultViewPort, vp, sizeof (struct ViewPort));

    AROS_LIBFUNC_EXIT
    
} /* InitViewPort */
