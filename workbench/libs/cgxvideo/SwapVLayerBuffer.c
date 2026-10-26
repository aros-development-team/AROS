/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH1(void, SwapVLayerBuffer,

/*  SYNOPSIS */
        AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
        struct Library *, CGXVideoBase, 16, Cgxvideo)

/*  FUNCTION
        Swaps the displayed and the rendering buffer of a double buffered
        video layer at the next vertical blank. The next LockVLayer() may
        wait for that vertical blank.

    INPUTS
        VLayerHandle - pointer to a previously created videolayer handle

    RESULT
        none

    NOTES
        Available since V50. Does nothing for a single buffered layer or
        while the layer is locked.

    EXAMPLE

    BUGS

    SEE ALSO
        LockVLayer(), QueryVLayerAttr()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct VLayerHandle *vh = VLayerHandle;

    if (!vh)
        return;

    ObtainSemaphore(&vh->lock);
    cgxv_Refresh(vh);
    if (!(vh->flags & VHF_NULL) && !vh->locked && vh->buffers > 1)
    {
        IPTR back = 0;

        OOP_GetAttr(vh->bm, aHidd_BitMap_BackBuffer, &back);
        HIDD_BM_SwapBuffers(vh->bm, back, 0);
    }
    ReleaseSemaphore(&vh->lock);

    AROS_LIBFUNC_EXIT
} /* SwapVLayerBuffer */
