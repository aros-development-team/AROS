/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$

    Desc: AROS Graphics function CreateRastPort()
    Lang: english
*/
#include "graphics_intern.h"
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <clib/graphics_protos.h>

	AROS_LH0(struct RastPort *, CreateRastPort,

/*  SYNOPSIS */

/*  LOCATION */
	struct GfxBase *, GfxBase, 177, Graphics)

/*  FUNCTION
	This function creates a new RastPort.

    INPUTS
	None.

    RESULT
	A pointer to a new RastPort or NULL if there was not enough memory
	to perform the operation.

    NOTES
	This function is AROS specific. For compatibility, there is a
	function in aros.lib which does the same on Amiga.

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
    struct RastPort * newRP;

    newRP = AllocMem (sizeof (struct RastPort), MEMF_ANY);

    if (newRP)
    {
	if (!InitRastPort (newRP))
	{
	    FreeMem (newRP, sizeof (struct RastPort));
	    newRP = NULL;
	}
    }

    return newRP;
    AROS_LIBFUNC_EXIT
} /* CreateRastPort */
