/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "arosmesa_funcs_gallium.h"
#include <proto/exec.h>
#include <aros/debug.h>
#include <proto/gallium.h>
#include <gallium/gallium.h>
#include <gallium/pipe/p_context.h>
#include <gallium/pipe/p_screen.h>

GLAContext glACreateContext(struct TagItem *tagList);

/*****************************************************************************

    NAME */

    APTR AROSMesaCreateContextTags(

/*  SYNOPSIS */
	long Tag1,
	...)

/*  FUNCTION
        This is the varargs version of mesa.library/AROSMesaCreateContext().
        For information see mesa.library/AROSMesaCreateContext().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        mesa.library/AROSMesaCreateContext()

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_SLOWSTACKTAGS_PRE_AS(Tag1, GLAContext)
  retval = glACreateContext(AROS_SLOWSTACKTAGS_ARG(Tag1));
  AROS_SLOWSTACKTAGS_POST
}

APTR AROSMesaCreateContext(struct TagItem *tagList)
{
    return glACreateContext(tagList);
}


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

        A valid GL context or NULL of creation was not succesfull.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    struct arosmesa_context * amesa = NULL;
    struct pipe_screen * pscreen = NULL;
    struct st_context_attribs attribs = {0};

    /* Allocate arosmesa_context struct initialized to zeros */
    if (!(amesa = (struct arosmesa_context *)AllocVec(sizeof(struct arosmesa_context), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR - failed to allocate AROSMesaContext\n"));
        return NULL;
    }
    
    AROSMesaSelectRastPort(amesa, tagList);
    if (!amesa->visible_rp)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR - failed to select visible rastport\n"));
        goto error_out;
    }    
    
    AROSMesaStandardInit(amesa, tagList);   

    pscreen = CreatePipeScreenV(NULL);
    if (!pscreen)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create gallium pipe screen\n"));
        goto error_out;
    }

    if (!(amesa->stmanager = AROSMesaNewStManager(pscreen)))
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR - failed to create ST Manager\n"));
        DestroyPipeScreen(pscreen);
        goto error_out;
    }

    D(bug("[AROSMESA] AROSMesaCreateContext: Filling ST Visual \n"));
    if (!AROSMesaFillVisual(&amesa->stvis, amesa->stmanager->screen, amesa->BitsPerPixel, tagList))
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to fill ST Visual\n"));
        goto error_out;
    }

    attribs.profile = ST_PROFILE_DEFAULT;
    attribs.visual = amesa->stvis;
   
    amesa->st = glstapi->create_context(glstapi, amesa->stmanager, &attribs, NULL);
    if (!amesa->st)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create mesa state tracker context\n"));
        goto error_out;
    }
    
    amesa->framebuffer = AROSMesaNewFrameBuffer(amesa, &amesa->stvis);

    if (!amesa->framebuffer)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create frame buffer\n"));
        goto error_out;
    }
    
    return amesa;

error_out:
    if (amesa->stmanager) AROSMesaFreeStManager(amesa->stmanager);
    if (amesa) AROSMesaFreeContext(amesa);
    return NULL;
}
