/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/gallium.h>

#include "mesa3dgl_support.h"
#include "mesa3dgl_gallium.h"


/*****************************************************************************

    NAME */

      void glADestroyContext(

/*  SYNOPSIS */
      GLAContext ctx)

/*  FUNCTION
        Destroys the GL rendering context and frees all resoureces.

    INPUTS
        ctx - pointer to GL rendering context. A NULL pointer will be
                ignored.

    RESULT
        The GL context is destroyed. Do no use it anymore.

    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    struct mesa3dgl_context * _ctx = (struct mesa3dgl_context *)ctx;

    D(bug("[MESA3DGL] %s(ctx @ %x)\n", __func__, ctx));

    if (_ctx)
    {
        struct st_context * st_ctx = _ctx->st;

        if (st_ctx)
        {
            struct st_context * cur_ctx = st_api_get_current();

            if (cur_ctx == st_ctx)
            {
                st_context_flush(_ctx->st, 0, NULL, NULL, NULL);
                st_api_make_current(NULL, NULL, NULL);
            }

            /* the state tracker keeps per-drawable state; drop it before
             * the drawable memory goes away */
            if (_ctx->framebuffer)
                st_api_destroy_drawable(&_ctx->framebuffer->base);
            st_destroy_context(_ctx->st);
            MESA3DGLFreeFrameBuffer(_ctx->framebuffer);
            MESA3DGLFreeStManager(_ctx->driver, _ctx->stmanager);
            MESA3DGLFreeContext(_ctx);
        }
    }
}
