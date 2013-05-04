/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <aros/debug.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	AROS_LH1(ULONG, DetachVLayer,

/*  SYNOPSIS */
	AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
	struct Library *, CGXVideoBase, 8, Cgxvideo)

/*  FUNCTION
	Detaches a videolayer from a given window. As a result, the video
	overlay should now be unlinked from the window and the original
	contents of the window are visible now.

    INPUTS
	VLayerHandle - pointer to a previously created videolayer handle

    RESULT
	result - 0 if videolayer could be detached from the window

    NOTES

    EXAMPLE

    BUGS
        This function is unimplemented.

    SEE ALSO
	AttachVLayerTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    aros_print_not_implemented ("DetachVLayer");
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* DetachVLayer */
