/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH1(void, WaitBOVP,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 182, Graphics)

/*  FUNCTION

    INPUTS
	rp - pointer to rastport

    RESULT
	None.

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

    kprintf("Function not implemented: Graphics_WaitBOVP!\n");

    AROS_LIBFUNC_EXIT
} /* WaitBOVP */
