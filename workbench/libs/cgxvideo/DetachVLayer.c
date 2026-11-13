/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
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
        The backfill hook installed by AttachVLayerTagList() is removed;
        the key colour stays in the window until the application redraws.

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

    struct IntCGXVBase *base = GetCGXVBase(CGXVideoBase);
    struct VLayerHandle *vh = VLayerHandle;
    ULONG result = TRUE;

    if (!vh)
        return result;

    ObtainSemaphore(&base->handleLock);
    ObtainSemaphore(&vh->lock);
    if (!vh->locked && (vh->flags & VHF_ATTACHED))
    {
        cgxv_Detach(vh);
        result = 0;
    }
    ReleaseSemaphore(&vh->lock);
    ReleaseSemaphore(&base->handleLock);

    return result;

    AROS_LIBFUNC_EXIT
} /* DetachVLayer */
