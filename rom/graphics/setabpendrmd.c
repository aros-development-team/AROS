/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function SetABPenDrMd()
    Lang: english
*/
#include "graphics_intern.h"
#include <proto/oop.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH4(void, SetABPenDrMd,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(ULONG            , apen, D0),
	AROS_LHA(ULONG            , bpen, D1),
	AROS_LHA(ULONG            , drawMode, D2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 149, Graphics)

/*  FUNCTION
	Changes the foreground and background pen and the drawmode in one
	step.

    INPUTS
	rp - Modify this RastPort
	apen - The new foreground pen
	bpen - The new background pen
	drawmode - The new drawmode

    RESULT
	None.

    NOTES
	This function is faster than the sequence SetAPen(), SetBPen(),
	SetDrMd().

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

    struct gfx_driverdata *dd;
    
    if (CorrectDriverData(rp, GfxBase))
    {

	dd = GetDriverData(rp);
	if (dd)
	{
    	    struct TagItem gc_tags[] =
	    {
    		{ aHidd_GC_Foreground	    	, 0 	    	    	},
    		{ aHidd_GC_Background	    	, 0 	    	    	},
		{ aHidd_GC_ColorExpansionMode	, 0UL	    	    	},
		{ aHidd_GC_DrawMode 	    	, vHidd_GC_DrawMode_Copy},
		{ TAG_DONE  	    	    		    	    	}
    	    };

	    gc_tags[0].ti_Data = BM_PIXEL(rp->BitMap, apen & PEN_MASK);
	    gc_tags[1].ti_Data = BM_PIXEL(rp->BitMap, bpen & PEN_MASK);

	    if (drawMode & JAM2)
	    {
		gc_tags[2].ti_Data = vHidd_GC_ColExp_Opaque;
	    }
	    else if (drawMode & COMPLEMENT)
	    {
		gc_tags[3].ti_Data = vHidd_GC_DrawMode_Invert;
	    }
	    else if ((drawMode & (~INVERSVID)) == JAM1)
	    {
		gc_tags[2].ti_Data = vHidd_GC_ColExp_Transparent;
	    }

    	    OOP_SetAttrs(dd->dd_GC, gc_tags);

	}

    } /* if (CorrectDriverData (rp, GfxBase)) */
    
    /* Do it after the driver to allow it to inspect the previous value */
    rp->FgPen = apen;
    rp->BgPen = bpen;
    rp->DrawMode = drawMode;

    AROS_LIBFUNC_EXIT
} /* SetABPenDrMd */
