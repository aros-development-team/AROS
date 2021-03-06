/*
    Copyright 2011-2015, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/arossupport.h>

#include "hostgl_ctx_manager.h"
#include "hostgl_funcs.h"
#include "hostgl_support.h"

/*****************************************************************************

    NAME */

      void glASetRast(

/*  SYNOPSIS */
      GLAContext ctx,
      struct TagItem * tagList)

/*  FUNCTION

        Sets a new rendering target for an existing context
 
    INPUTS

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
    struct hostgl_context *_ctx = (struct hostgl_context *)ctx;

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
            HostGLSelectRastPort(_ctx, tagList);

            /* Do standard initialization */
            HostGLStandardInit(_ctx, tagList);

            /* TODO: what to do with visual and framebuffer, if BPP changes, we are in trouble */

            /* After the new render target has been attached, invoke framebuffer recalculation */
            HostGL_CheckAndUpdateBufferSize(_ctx);
        }
    }
}
