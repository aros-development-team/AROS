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

/*****************************************************************************

    NAME */

    AROSMesaContext AROSMesaCreateContextTags(

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
  AROS_SLOWSTACKTAGS_PRE_AS(Tag1, AROSMesaContext)
  retval = AROSMesaCreateContext(AROS_SLOWSTACKTAGS_ARG(Tag1));
  AROS_SLOWSTACKTAGS_POST
}



/*****************************************************************************

    NAME */

      AROSMesaContext AROSMesaCreateContext(

/*  SYNOPSIS */ 
      struct TagItem *tagList)

/*  FUNCTION

        Crates a GL rendering context. Whether the rendering will be software
        or hardware based depends on the gallium.library returning a module
        best suited.
 
    INPUTS

        tagList - a pointer to tags to be used during creation.
 
    TAGS

        AMA_Left   - specifies the left rendering offset on the rastport. 
                     Typically equals to window->BorderLeft.

        AMA_Top    - specifies the top rendering offset on the rastport. 
                     Typically equals to window->BorderTop.

        AMA_Right  - specifies the right rendering offset on the rastport. 
                     Typically equals to window->BorderRight.

        AMA_Bottom - specifies the bottom rendering offset on the rastport. 
                     Typically equals to window->BorderBottom.
    
        AMA_Width  - specifies the width of the rendering area. 
                     AMA_Width + AMA_Left + AMA_Right should equal the width of
                     the rastport. The AMA_Width is interchangable at cration 
                     time with AMA_Right. Later durring window resizing, width 
                     is calculated from scalled left, righ and window width.

        AMA_Height - specifies the height of the rendering area. 
                     AMA_Height + AMA_Top + AMA_Bottom should equal the height 
                     of the rastport. The AMA_Height is interchangable at 
                     cration time with AMA_Bottom. Later durring window resizing
                     , height is calculated from scalled top, bottom and window 
                     height.

        AMA_Screen - pointer to Screen onto which scene is to be rendered. When
                     selecting RastPort has lower priority than AMA_Window.

        AMA_Window - pointer to Window onto which scene is to be rendered. Must
                     be provided.

        AMA_RastPort - ignored. Use AMA_Window.

        AMA_DoubleBuf - ignored. All rendering is always double buffered.

        AMA_RGBMode - ignored. All rendering is done in RGB. Indexed modes are 
                      not supported.

        AMA_AlphaFlag - ignored. All rendering is done with alpha channel.

        AMA_NoDepth - disables the depth/Z buffer. Depth buffer is enabled by
                      default and is 16 or 24 bit based on rendering 
                      capabilities.

        AMA_NoStencil - disables the stencil buffer. Stencil buffer is enabled
                        by default.

        AMA_NoAccum - disables the accumulation buffer. Accumulation buffer is
                      enabled by default.

    RESULT

        A valid GL context or NULL of creation was not succesfull.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROSMesaContext amesa = NULL;
    struct pipe_screen * pscreen = NULL;
    struct st_context_attribs attribs = {0};

    /* Allocate arosmesa_context struct initialized to zeros */
    if (!(amesa = (AROSMesaContext)AllocVec(sizeof(struct arosmesa_context), MEMF_PUBLIC | MEMF_CLEAR)))
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
