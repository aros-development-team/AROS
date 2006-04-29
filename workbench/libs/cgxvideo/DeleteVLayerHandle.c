/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <aros/debug.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	AROS_LH1(void, DeleteVLayerHandle,

/*  SYNOPSIS */
	AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
	struct Library *, CGXVideoBase, 6, Cgxvideo)

/*  FUNCTION
	Deletes a created video layer handle

    INPUTS
	VLayerHandle - pointer to a previously created videolayer handle

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CGXVideoBase)

    aros_print_not_implemented ("DeleteVLayerHandle");

    AROS_LIBFUNC_EXIT
} /* DeleteVLayerHandle */
