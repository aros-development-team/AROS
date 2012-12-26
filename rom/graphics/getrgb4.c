/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetRGB4()
    Lang: english
*/
#include <graphics/view.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(ULONG, GetRGB4,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, colormap, A0),
	AROS_LHA(LONG             , entry   , D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 97, Graphics)

/*  FUNCTION
	Read a value from the ColorMap. Use this function, as the colormap
	is subject to change.

    INPUTS
	colormap - pointer to ColorMap structure
	entry	 - index into colormap

    RESULT
	-1 	: if no valid entry. (index too high)
	other	: UWORD RGB value, 4 bits per electron gun, right justified

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetColorMap(), FreeColorMap(), SetRGB4(), LoadRGB4(), graphics/view.h

    INTERNALS
	This function depends on the structure of ColorMap->ColorTable.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    UWORD * CT;

    /* anything invalid?  */
    if ( (NULL == colormap) || (entry >= colormap->Count) )
        return -1L;

    /* All we're currently doing is read the entry and return it. */
    CT = colormap->ColorTable;
    return CT[entry];

    AROS_LIBFUNC_EXIT
    
} /* GetRGB4 */
