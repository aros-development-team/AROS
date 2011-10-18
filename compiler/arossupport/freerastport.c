/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS Graphics function FreeRastPort
    Lang: english
*/

#include <graphics/rastport.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/arossupport.h>

	void FreeRastPort(

/*  SYNOPSIS */
	struct RastPort *rp)

/*  FUNCTION
	This frees a RastPort obtained with CloneRastPort() or
	CreateRastPort().

    INPUTS
	rp - The result of CloneRastPort() or CreateRastPort().

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloneRastPort(), CreateRastPort()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    if (rp->RP_Extra)
    	FreeVec(rp->RP_Extra);

    FreeMem (rp, sizeof (struct RastPort));

} /* FreeRastPort */
