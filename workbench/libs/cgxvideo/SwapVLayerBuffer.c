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
        Shows the buffer rendered last at the next vertical blank. With two
        buffers the call returns once the displaced buffer is off the
        display. With three (VOA_MultiBuffer) it returns at once, and the
        buffer the next LockVLayer() hands out is neither shown nor waiting
        to be: if the display has not taken the previous buffer yet, the
        one just rendered is not shown and is handed out again, so the
        newest frame goes up at the next swap after the display is
        ready.

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
        /* with two, the buffer rendered next is the one displaced */
        HIDD_BM_SwapBuffers(vh->bm, back, (vh->buffers >= 3) ? fHidd_Swap_NoWait : 0);
    }
    ReleaseSemaphore(&vh->lock);

    AROS_LIBFUNC_EXIT
} /* SwapVLayerBuffer */
