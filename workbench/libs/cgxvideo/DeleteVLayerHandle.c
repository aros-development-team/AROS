/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <aros/debug.h>
#include <hidd/graphics.h>

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

    HIDD_Gfx_DisposeOverlay(VLayerHandle->drv, VLayerHandle->obj);

    AROS_LIBFUNC_EXIT
} /* DeleteVLayerHandle */
