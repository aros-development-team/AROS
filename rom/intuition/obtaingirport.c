/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Intuition Function ObtainGIRPort()
    Lang: english
*/
#include "intuition_intern.h"
#include <clib/graphics_protos.h>

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <intuition/cghooks.h>
	#include <clib/intuition_protos.h>

	AROS_LH1(struct RastPort *, ObtainGIRPort,

/*  SYNOPSIS */
	AROS_LHA(struct GadgetInfo *, gInfo, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 93, Intuition)

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
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    return CloneRastPort (gInfo->gi_RastPort);

    AROS_LIBFUNC_EXIT
} /* ObtainGIRPort */
