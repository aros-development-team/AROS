/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <proto/exec.h>

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
        Fails while the layer is locked or not attached.

    SEE ALSO
        AttachVLayerTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG result = TRUE;

    if (!VLayerHandle)
        return result;

    ObtainSemaphore(&VLayerHandle->lock);
    if (!VLayerHandle->locked && VLayerHandle->window)
    {
        VLayerHandle->window = NULL;
        result = 0;
    }
    ReleaseSemaphore(&VLayerHandle->lock);

    return result;

    AROS_LIBFUNC_EXIT
} /* DetachVLayer */
