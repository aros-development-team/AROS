/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

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
        The handle may still be locked or attached: it is unlocked and
        detached first.

    EXAMPLE

    BUGS

    SEE ALSO
        CreateVLayerHandleTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntCGXVBase *base = GetCGXVBase(CGXVideoBase);
    struct VLayerHandle *vh = VLayerHandle;

    if (!vh)
        return;

    ObtainSemaphore(&base->handleLock);
    ObtainSemaphore(&vh->lock);
    cgxv_UnlockBuffer(vh);
    cgxv_Detach(vh);
    ReleaseSemaphore(&vh->lock);
    ReleaseSemaphore(&base->handleLock);

    if (vh->bm)
        OOP_DisposeObject(vh->bm);
    if (vh->colorkeypen >= 0)
        ReleasePen(vh->screen->ViewPort.ColorMap, vh->colorkeypen);
    st_Cleanup(&vh->track);
    FreeVec(vh->nullbuffer);
    FreeMem(vh, sizeof(struct VLayerHandle));

    AROS_LIBFUNC_EXIT
} /* DeleteVLayerHandle */
