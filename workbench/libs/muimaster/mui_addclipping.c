/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "support.h"

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef __AROS
__asm APTR MUI_AddClipping(register __a0 struct MUI_RenderInfo *mri, register __d0 WORD left, register __d1 WORD top, register __d2 WORD width, register __d3 WORD height)
#else
	AROS_LH5(APTR, MUI_AddClipping,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(WORD, left, D0),
	AROS_LHA(WORD, top, D1),
	AROS_LHA(WORD, width, D2),
	AROS_LHA(WORD, height, D3),

/*  LOCATION */
	struct Library *, MUIMasterBase, 24, MUIMaster)
#endif

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *,MUIMasterBase)

    struct Rectangle rect;
    struct Region *r;

    if ((width >= MUI_MAXMAX) || (height >= MUI_MAXMAX))
        return (APTR)-1;

    if (mri->mri_rCount > 0)
    {
	if (isRegionWithinBounds(mri->mri_rArray[mri->mri_rCount-1], left, top, width, height))
	    return (APTR)-1;
    }

    if ((r = NewRegion()) == NULL)
        return (APTR)-1;

    rect.MinX = left;
    rect.MinY = top;
    rect.MaxX = left + width  - 1;
    rect.MaxY = top  + height - 1;
    OrRectRegion(r, &rect);

    return MUI_AddClipRegion(mri, r);

    AROS_LIBFUNC_EXIT

} /* MUIA_AddClipping */
