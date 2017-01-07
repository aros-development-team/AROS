/*
    Copyright 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/gallium.h>

#include <gallium/pipe/p_screen.h>
#include <gallium/util/u_inlines.h>

#include "mesa3dgl_types.h"
#include "mesa3dgl_support.h"
#include "mesa3dgl_gallium.h"


/*****************************************************************************

    NAME */

      void glASwapBuffers(

/*  SYNOPSIS */
      GLAContext ctx)

/*  FUNCTION
        Swaps the back with front buffers. MUST BE used to display the effect
        of rendering onto the target RastPort, since GLA always work in
        double buffer mode.

    INPUTS
        ctx - GL rendering context on which swap is to be performed.

    RESULT

    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    struct mesa3dgl_context *_ctx = (struct mesa3dgl_context *)ctx;

    if (_ctx->framebuffer->render_resource) 
    {
        /* Flush rendering cache before blitting */
        _ctx->st->flush(_ctx->st, ST_FLUSH_FRONT, NULL);

        BltPipeResourceRastPort(_ctx->driver, _ctx->framebuffer->render_resource, 0, 0, 
            _ctx->visible_rp, _ctx->left, _ctx->top, 
            _ctx->framebuffer->width, _ctx->framebuffer->height);
    }

    MESA3DGLCheckAndUpdateBufferSize(_ctx);
}
