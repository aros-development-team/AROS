/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/29 13:33:30  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.1  1996/08/23 17:28:18  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <intuition/intuition.h>
	#include <clib/intuition_protos.h>

	__AROS_LH4(void, DrawImage,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A0),
	__AROS_LHA(struct Image    *, image, A1),
	__AROS_LHA(long             , leftOffset, D0),
	__AROS_LHA(long             , topOffset, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 19, Intuition)

/*  FUNCTION
	Draw an image.

    INPUTS
	rp - The RastPort to render into
	image - The image to render
	leftOffset, topOffset - Where to place the image.

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    __AROS_FUNC_EXIT
} /* DrawImage */
