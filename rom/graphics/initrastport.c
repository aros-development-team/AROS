/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <string.h>

void driver_InitRastPort (struct RastPort *, struct GfxBase *);

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH1(void, InitRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 33, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    memset (rp, 0, sizeof (struct RastPort));

    driver_InitRastPort (rp, GfxBase);

    AROS_LIBFUNC_EXIT
} /* InitRastPort */
