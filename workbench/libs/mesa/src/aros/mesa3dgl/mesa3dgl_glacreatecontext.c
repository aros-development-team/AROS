/*
    Copyright 2009-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/gallium.h>

#include <gallium/gallium.h>
#include <gallium/pipe/p_context.h>
#include <gallium/pipe/p_screen.h>

#include "mesa3dgl_support.h"
#include "mesa3dgl_gallium.h"

/*****************************************************************************

    NAME */

      GLAContext glACreateContext(

/*  SYNOPSIS */ 
      struct TagItem *tagList)

/*  FUNCTION

        Crates a GL rendering context that can be later used in subsequent
        calls.
 
    INPUTS

        tagList - a pointer to tags to be used during creation.
 
    TAGS

        GLA_Left   - specifies the left rendering offset on the rastport.
                     Typically equals to window->BorderLeft.

        GLA_Top    - specifies the top rendering offset on the rastport.
                     Typically equals to window->BorderTop.

        GLA_Right  - specifies the right rendering offset on the rastport.
                     Typically equals to window->BorderRight.

        GLA_Bottom - specifies the bottom rendering offset on the rastport.
                     Typically equals to window->BorderBottom.
    
        GLA_Width  - specifies the width of the rendering area.
                     GLA_Width + GLA_Left + GLA_Right should equal the width of
                     the rastport. The GLA_Width is interchangable at cration
                     time with GLA_Right. Later durring window resizing, width
                     is calculated from scalled left, righ and window width.

        GLA_Height - specifies the height of the rendering area.
                     GLA_Height + GLA_Top + GLA_Bottom should equal the height
                     of the rastport. The GLA_Height is interchangable at
                     cration time with GLA_Bottom. Later durring window resizing
                     , height is calculated from scalled top, bottom and window 
                     height.

        GLA_Screen - pointer to Screen onto which scene is to be rendered. When
                     selecting RastPort has lower priority than GLA_Window.

        GLA_Window - pointer to Window onto which scene is to be rendered. Must
                     be provided.

        GLA_RastPort - ignored. Use GLA_Window.

        GLA_DoubleBuf - ignored. All rendering is always double buffered.

        GLA_RGBMode - ignored. All rendering is done in RGB. Indexed modes are
                      not supported.

        GLA_AlphaFlag - ignored. All rendering is done with alpha channel.

        GLA_NoDepth - disables the depth/Z buffer. Depth buffer is enabled by
                      default and is 16 or 24 bit based on rendering 
                      capabilities.

        GLA_NoStencil - disables the stencil buffer. Stencil buffer is enabled
                        by default.

        GLA_NoAccum - disables the accumulation buffer. Accumulation buffer is
                      enabled by default.

    RESULT

        A valid GL context or NULL of creation was not successful.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    struct mesa3dgl_context * ctx = NULL;
    struct pipe_screen * pscreen = NULL;
    struct st_context_attribs attribs = {0};

    /* Allocate MESA3DGL context */
    if (!(ctx = (struct mesa3dgl_context *)AllocVec(sizeof(struct mesa3dgl_context), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        bug("%s: ERROR - failed to allocate GLAContext\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    MESA3DGLSelectRastPort(ctx, tagList);
    if (!ctx->visible_rp)
    {
        bug("%s: ERROR - failed to select visible rastport\n", __PRETTY_FUNCTION__);
        goto error_out;
    }    

    MESA3DGLStandardInit(ctx, tagList);   

    pscreen = CreatePipeScreenV(NULL);
    if (!pscreen)
    {
        bug("%s: ERROR -  failed to create gallium pipe screen\n", __PRETTY_FUNCTION__);
        goto error_out;
    }

    if (!(ctx->stmanager = MESA3DGLNewStManager(pscreen)))
    {
        bug("%s: ERROR - failed to create ST Manager\n");
        DestroyPipeScreen(pscreen);
        goto error_out;
    }

    D(bug("[MESA3DGL] %s: Filling ST Visual \n", __PRETTY_FUNCTION__));
    if (!MESA3DGLFillVisual(&ctx->stvis, ctx->stmanager->screen, ctx->BitsPerPixel, tagList))
    {
        bug("%s: ERROR -  failed to fill ST Visual\n", __PRETTY_FUNCTION__);
        goto error_out;
    }

    attribs.profile = ST_PROFILE_DEFAULT;
    attribs.visual = ctx->stvis;

    ctx->st = glstapi->create_context(glstapi, ctx->stmanager, &attribs, NULL);
    if (!ctx->st)
    {
        bug("%s: ERROR -  failed to create mesa state tracker context\n", __PRETTY_FUNCTION__);
        goto error_out;
    }

    ctx->framebuffer = MESA3DGLNewFrameBuffer(ctx, &ctx->stvis);

    if (!ctx->framebuffer)
    {
        bug("%s: ERROR -  failed to create frame buffer\n", __PRETTY_FUNCTION__);
        goto error_out;
    }

    return (GLAContext)ctx;

error_out:
    if (ctx->stmanager) MESA3DGLFreeStManager(ctx->stmanager);
    if (ctx) MESA3DGLFreeContext(ctx);
    return (GLAContext)NULL;
}
