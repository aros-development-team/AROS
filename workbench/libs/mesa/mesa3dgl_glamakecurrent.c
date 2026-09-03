/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "mesa3dgl_types.h"
#include "mesa3dgl_support.h"

/*****************************************************************************

    NAME */

      void glAMakeCurrent(

/*  SYNOPSIS */
      GLAContext ctx)

/*  FUNCTION
        Make the selected GL rendering context active.

    INPUTS
        ctx - GL rendering context to be made active for all following GL
                calls.

    RESULT

    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    struct mesa3dgl_context *_ctx = (struct mesa3dgl_context *)ctx;

    D(bug("[MESA3DGL] %s()\n", __func__));

    if (_ctx)
    {
        struct st_context * cur_ctx = st_api_get_current();

        if (_ctx->st != cur_ctx)
        {
            /* Recalculate buffer dimensions */
            MESA3DGLRecalculateBufferWidthHeight(_ctx);

            D(bug("[MESA3DGL] %s: _ctx->framebuffer @ 0x%p base @ 0x%p\n", __func__, _ctx->framebuffer, &_ctx->framebuffer->base));
            /* Attach */
            st_api_make_current(_ctx->st,
                &_ctx->framebuffer->base, &_ctx->framebuffer->base);
        }
    }
    else
    {
        /* Detach */
        st_api_make_current(NULL, NULL, NULL);
    }
    D(bug("[MESA3DGL] %s: done\n", __func__));
}
