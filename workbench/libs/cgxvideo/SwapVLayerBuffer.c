/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>

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
        Available since V50.

    EXAMPLE

    BUGS
        No backend supports double buffering yet, so this does nothing.

    SEE ALSO
        LockVLayer(), QueryVLayerAttr()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
} /* SwapVLayerBuffer */
