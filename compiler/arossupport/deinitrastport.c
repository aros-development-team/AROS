/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: AROS Graphics function DeinitRastPort()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/rastport.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

	void DeinitRastPort(

/*  SYNOPSIS */
	struct RastPort *rp)

/*  FUNCTION

        Frees the extra contents associated with a RastPort structure.
        The structure itself is not freed.

    INPUTS

        rp - The RastPort which contents are to be freed.

    RESULT

        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

        graphics.library/InitRastPort()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    D(bug("DeInitRastPort()\n"));

    if (rp->RP_Extra)
    {
    	FreeVec(rp->RP_Extra);
    	rp->RP_Extra = NULL;
    }

} /* DeinitRastPort */
