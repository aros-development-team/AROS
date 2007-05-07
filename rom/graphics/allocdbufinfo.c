/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */

#include <graphics/view.h>

    AROS_LH1(struct DBufInfo *, AllocDBufInfo,

/*  SYNOPSIS */

	AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */

	struct GfxBase *, GfxBase, 161, Graphics)

/*  FUNCTION

    Allocates a double buffering structure used by ChangeVPBitMap().

    INPUTS

    vp  -  pointer to a ViewPort

    RESULTS

    Returns NULL if there wasn't enough memory (or if the viewport doesn't
    support double buffering).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    Which ViewPorts doesn't support double buffering?

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    return (struct DBufInfo *)AllocMem(MEMF_ANY | MEMF_CLEAR,
				       sizeof(struct DBufInfo));
    
    AROS_LIBFUNC_EXIT
} /* AllocDBufInfo */

