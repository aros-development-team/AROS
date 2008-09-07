/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetColorMap()
    Lang: english
*/
#include "exec/memory.h"
#include "exec/types.h"
#include "proto/exec.h"
#include "graphics/view.h"
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, FreeColorMap,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, colormap, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 96, Graphics)

/*  FUNCTION
	Correctly frees a ColorMap structure and associated structures
        that have previously been allocated via GetColorMap().

    INPUTS
	colormap - pointer to the ColorMap structure previously
		   allocated via GetColorMap().

    RESULT
	Memory returned to pool of free memory.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetColorMap, SetRGB4(), graphics/view.h

    INTERNALS
	Also frees a possibly connected PaletteExtra and ViewPortExtra	
	structure. Possibly wrong, though.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* if colormap != NULL return the memory */
    if (NULL != colormap)
    {
	/* free the ColorTable */
	FreeMem(colormap->ColorTable  , (colormap->Count) * sizeof(UWORD));

	/* free the LowColorBits */
	FreeMem(colormap->LowColorBits, (colormap->Count) * sizeof(UWORD));

	/* free a ViewPortExtra structure that might be connected to this one */
	if (NULL != colormap->cm_vpe)
	    GfxFree((struct ExtendedNode *)colormap->cm_vpe);

	/* free a PaletteExtra structure that might be connected to this */
	if (NULL != colormap->PalExtra)
	{
	    FreeMem(colormap->PalExtra->pe_RefCnt   , colormap->Count * sizeof(PalExtra_RefCnt_Type));
	    FreeMem(colormap->PalExtra->pe_AllocList, colormap->Count * sizeof(PalExtra_AllocList_Type));
	    FreeMem(colormap->PalExtra, sizeof(struct PaletteExtra));
	}
	/* free the structure itself */
	FreeMem(colormap, sizeof(struct ColorMap));
    }

    AROS_LIBFUNC_EXIT
    
} /* FreeColorMap */
