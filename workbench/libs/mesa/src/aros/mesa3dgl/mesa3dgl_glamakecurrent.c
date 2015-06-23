/*
    Copyright 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

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

    if (_ctx)
    {
        struct st_context_iface * cur_ctx = glstapi->get_current(glstapi);

        if (_ctx->st != cur_ctx)
        {
            /* Recalculate buffer dimensions */
            MESA3DGLRecalculateBufferWidthHeight(_ctx);

            /* Attach */
            glstapi->make_current(glstapi, _ctx->st, 
                &_ctx->framebuffer->base, &_ctx->framebuffer->base);
        }
    }
    else
    {
        /* Detach */
        glstapi->make_current(glstapi, NULL, NULL, NULL);
    }
}
