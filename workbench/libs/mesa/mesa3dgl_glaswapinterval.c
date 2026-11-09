/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include "mesa3dgl_support.h"
#include "mesa3dgl_gallium.h"

/*****************************************************************************

    NAME */

      void glASwapInterval(

/*  SYNOPSIS */
      GLAContext ctx,
      GLint interval)

/*  FUNCTION
        Sets how glASwapBuffers() paces a context's frames.

    INPUTS
        ctx      - GL context
        interval - 0: swaps return at once; rendering is not tied to the
                   display's refresh, and a frame finished before the
                   display took the previous one is not shown.
                   1 (or more): every swap waits until its frame is on
                   screen, so frames follow the refresh rate.

    RESULT
        None. Takes effect at the next glASwapBuffers().

    NOTES
        Applies to windows shown directly from the rendered buffers (3D
        scan-out); a window that is copied on swap is never paced.

    SEE ALSO
        glACreateContext() (GLA_SwapInterval)

*****************************************************************************/
{
    struct mesa3dgl_context *_ctx = (struct mesa3dgl_context *)ctx;

    D(bug("[MESA3DGL] %s(%d)\n", __func__, (int)interval));

    if (_ctx)
        _ctx->swap_interval = interval > 0 ? 1 : 0;
}
