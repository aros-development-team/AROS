/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for allocating screen bitmaps
    Lang: english
*/
#include "graphics_intern.h"
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <oop/oop.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH1(struct BitMap * , AllocScreenBitMap,

/*  SYNOPSIS */
	AROS_LHA(ULONG, modeid, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 182, Graphics)

/*  FUNCTION
	Allocates a bitmap for use with a screen opened by OpenScreen()

    INPUTS
	modeid - the DisplayID of the screen to create

    RESULT
    	bitmap - pointer to the newly created bitmap.

    NOTES
	This function is private and AROS specific.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    return driver_AllocScreenBitMap(modeid, GfxBase);

    AROS_LIBFUNC_EXIT
} /* AllocScreenBitMap */
