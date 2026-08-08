/*
    Copyright (C) 2009-2020, The AROS Development Team. All rights reserved.
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
        struct st_context_iface * st_ctx = _ctx->st;

        if (st_ctx)
        {
            struct st_context_iface * cur_ctx = glstapi->get_current(glstapi);

            if (cur_ctx == st_ctx)
            {
                _ctx->st->flush(_ctx->st, 0, NULL, NULL, NULL);
                glstapi->make_current(glstapi, NULL, NULL, NULL);
            }

            _ctx->st->destroy(_ctx->st);
            MESA3DGLFreeFrameBuffer(_ctx->framebuffer);
            MESA3DGLFreeStManager(_ctx->driver, _ctx->stmanager);
            /* glstapi is library-global (created once in Init, freed in
             * Expunge — mesa3dgl_init.c). Destroying it here left a
             * dangling pointer, so the NEXT glACreateContext in the same
             * session (fullscreen/window toggles, SDL's probe context)
             * ran create_context on freed memory: grey screens or
             * crashes depending on heap reuse. */
            MESA3DGLFreeContext(_ctx);
        }
    }
}
