/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function CloneRastPort()
    Lang: english
*/
#include "graphics_intern.h"
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH1(struct RastPort *, CloneRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 178, Graphics)

/*  FUNCTION
	This function creates a copy of a RastPort.

    INPUTS
	rp - The RastPort to clone.

    RESULT
	A pointer to a RastPort with the same attributes as the RastPort
	which was specified or NULL if there was not enough memory to perform
	the operation.

    NOTES
	This function is AROS specific. For compatibility, there is a
	function in aros.lib which does the same on Amiga.

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct RastPort * newRP;

    newRP = AllocMem (sizeof (struct RastPort), MEMF_ANY);

    if (newRP)
    {
	CopyMem (rp, newRP, sizeof (struct RastPort));
	RP_BACKPOINTER(newRP) = newRP;
	RP_DRIVERDATA(newRP) = NULL;
    	newRP->Flags |= RPF_SELF_CLEANUP;
	
	if (!OBTAIN_DRIVERDATA(newRP,  GfxBase))
	{
	    FreeMem (newRP, sizeof (struct RastPort));
	    newRP = NULL;
	}
	else
	{
	    /* copy rastports attributes */
	    SetFont(newRP, rp->Font);
	    SetABPenDrMd(newRP, GetAPen(rp), GetBPen(rp), GetDrMd(rp));
	    Move(newRP, rp->cp_x, rp->cp_y);

	    #warning Some attributes not copied 
	    
	    RELEASE_DRIVERDATA(newRP, GfxBase);   
 	}
    }

    return newRP;
    
    AROS_LIBFUNC_EXIT
    
} /* CloneRastPort */
