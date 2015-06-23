/*
    Copyright 2011-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#include "mesa3dgl_support.h"
#include "mesa3dgl_gallium.h"

/*****************************************************************************

    NAME */

      void glASetRast(

/*  SYNOPSIS */
      GLAContext ctx,
      struct TagItem * tagList)

/*  FUNCTION

        Sets a new rendering target for an existing context

    INPUTS

        ctx - 
        tagList - a pointer to tags to be used during creation.

    TAGS

        GLA_Window - pointer to Window onto which scene is to be rendered. Must
                     be provided.

    RESULT

        None

    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    struct mesa3dgl_context * _ctx = (struct mesa3dgl_context *)ctx;

    if (_ctx)
    {
        /* Check if at least one of window, rastport or screen have been passed */
        if ((GetTagData(GLA_Screen, 0, tagList) != 0) ||
            (GetTagData(GLA_Window, 0, tagList) != 0) ||
            (GetTagData(GLA_RastPort, 0, tagList) != 0))
        {
            /* If there already is visible_rp, free it */
            if (_ctx->visible_rp)
                FreeRastPort(_ctx->visible_rp);
            /* Do standard rast port selection */
            MESA3DGLSelectRastPort(_ctx, tagList);

            /* Do standard initialization */
            MESA3DGLStandardInit(_ctx, tagList); 

            /* TODO: what to do with visual and framebuffer, if BPP changes, we are in trouble */

            /* After the new render target has been attached, invoke framebuffer recalculation */
            MESA3DGLCheckAndUpdateBufferSize(_ctx);
        }
    }
}
