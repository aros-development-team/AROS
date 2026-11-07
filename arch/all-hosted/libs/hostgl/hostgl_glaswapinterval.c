/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "hostgl_ctx_manager.h"
#include <aros/debug.h>

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
        interval - 0: swaps return at once. 1 (or more): every swap waits
                   until its frame is on screen.

    RESULT
        None.

    NOTES
        HostGL's swaps copy the host's rendering into the window, and a
        window that is copied on swap is never paced: the interval is kept
        but has no effect.

    SEE ALSO
        glACreateContext() (GLA_SwapInterval)

*****************************************************************************/
{
    struct hostgl_context *_ctx = (struct hostgl_context *)ctx;

    D(bug("[HostGL] %s(%d)\n", __func__, (int)interval));

    if (_ctx)
        _ctx->swap_interval = interval > 0 ? 1 : 0;
}
