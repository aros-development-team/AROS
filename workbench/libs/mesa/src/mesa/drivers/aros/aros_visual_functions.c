/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: aros_visual_functions.c 31775 2009-09-06 16:49:16Z deadwood $
*/

#include "aros_visual_functions.h"


#include <aros/debug.h>
#include <exec/types.h>
#include <proto/utility.h>
#include "context.h"

AROSMesaVisual aros_new_visual(struct TagItem *tagList)
{
    AROSMesaVisual aros_vis = NULL;
    GLvisual * vis = NULL;
    GLint  indexBits, redBits, greenBits, blueBits, alphaBits, depthBits, stencilBits, accumBits;
    
    D(bug("[AROSMESA] aros_new_visual\n"));

    /* Allocated memory for aros structure */
    aros_vis = AllocVec(sizeof(struct arosmesa_visual), MEMF_PUBLIC | MEMF_CLEAR);

    if (!aros_vis)
        return NULL;

    vis = GET_GL_VIS_PTR(aros_vis);

    /* Create core visual with default values */
    depthBits = DEFAULT_SOFTWARE_DEPTH_BITS;
    stencilBits = 8;
    accumBits = 16;
    indexBits = 0;
    redBits = CHAN_BITS;
    greenBits = CHAN_BITS;
    blueBits = CHAN_BITS;
    alphaBits = CHAN_BITS;
    
    /* Override default values */
    /* AMA_RGBMode, AMA_DoubleBuf and AMA_AlphaFlag are always GL_TRUE in this implementation */
    stencilBits     = !GetTagData(AMA_NoStencil, GL_FALSE, tagList) ? stencilBits : 0;
    accumBits       = !GetTagData(AMA_NoAccum, GL_FALSE, tagList) ? accumBits : 0;
    depthBits       = !GetTagData(AMA_NoDepth, GL_FALSE, tagList) ? depthBits : 0;

    /* Initialize mesa structure */
    if(!_mesa_initialize_visual(vis,
                                GL_TRUE,        /* RGB */
                                GL_FALSE,       /* Double buffer - AROSMesa uses front buffer as back buffer */
                                GL_FALSE,       /* stereo */
                                redBits,
                                greenBits,
                                blueBits,
                                alphaBits,
                                indexBits,
                                depthBits,
                                stencilBits,
                                accumBits,
                                accumBits,
                                accumBits,
                                alphaBits ? accumBits : 0,
                                1))
    {
        _aros_destroy_visual(aros_vis);
        return NULL;
    }

    return aros_vis;
}

void _aros_destroy_visual(AROSMesaVisual aros_vis)
{
    if (aros_vis)
    {
        FreeVec(aros_vis);
    }
}
