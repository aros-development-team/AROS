/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Intuition function ReleaseGIRPort()
    Lang: english
*/
#include "intuition_intern.h"
#include <clib/graphics_protos.h>

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <clib/intuition_protos.h>

	AROS_LH1(void, ReleaseGIRPort,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 94, Intuition)

/*  FUNCTION
	Release a RastPort previously obtained by ObtainGIRPort().

    INPUTS
	rp - The result of ObtainGIRPort()

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    FreeRastPort (rp);

    AROS_LIBFUNC_EXIT
} /* ReleaseGIRPort */
