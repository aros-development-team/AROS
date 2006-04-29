/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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

    SEE ALSO
	AttachVLayerTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CGXVideoBase)

    aros_print_not_implemented ("DetachVLayer");

    AROS_LIBFUNC_EXIT
} /* DetachVLayer */
