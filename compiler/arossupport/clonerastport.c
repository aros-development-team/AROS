/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function CloneRastPort()
    Lang: english
*/

#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/arossupport.h>

	struct RastPort *CloneRastPort(

/*  SYNOPSIS */
	struct RastPort *rp)

/*  FUNCTION
	This function creates a copy of a RastPort.

    INPUTS
	rp - The RastPort to clone.

    RESULT
	A pointer to a RastPort with the same attributes as the RastPort
	which was specified or NULL if there was not enough memory to perform
	the operation.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreateRastPort(), FreeRastPort()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    struct RastPort *newRP = AllocMem (sizeof (struct RastPort), MEMF_ANY);

    if (newRP)
	CopyMem (rp, newRP, sizeof (struct RastPort));

    return newRP;

} /* CloneRastPort */
