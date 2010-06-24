/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "state_tracker/st_public.h"
#include <proto/exec.h>
#include <aros/debug.h>
#include <proto/gallium.h>
#include <gallium/gallium.h>
#include <gallium/pipe/p_context.h>

AROSMesaContext AROSMesaCreateContextTags(long Tag1, ...)
{
  AROS_SLOWSTACKTAGS_PRE(Tag1)
  retval = (IPTR)AROSMesaCreateContext(AROS_SLOWSTACKTAGS_ARG(Tag1));
  AROS_SLOWSTACKTAGS_POST
}

AROSMesaContext AROSMesaCreateContext(struct TagItem *tagList)
{
    AROSMesaContext amesa = NULL;
    struct pipe_context * pipe = NULL;
    
    /* Allocate arosmesa_context struct initialized to zeros */
    if (!(amesa = (AROSMesaContext)AllocVec(sizeof(struct arosmesa_context), MEMF_PUBLIC|MEMF_CLEAR)))
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR - failed to allocate AROSMesaContext\n"));
        return NULL;
    }
    
    /* FIXME; shouldn't RastPort be part of framebuffer? */
    AROSMesaSelectRastPort(amesa, tagList);
    
    /* FIXME: check if any rastport is available */
    
    /* FIXME: later this might be placed in initialization of framebuffer */
    AROSMesaStandardInit(amesa, tagList);   

    amesa->pscreen = CreatePipeScreenV(NULL);
    if (!amesa->pscreen)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create gallium pipe screen\n"));
        goto error_out;
    }
    
    D(bug("[AROSMESA] AROSMesaCreateContext: Creating new AROSMesaVisual\n"));
    amesa->visual = AROSMesaNewVisual(amesa->ScreenInfo.BitsPerPixel, amesa->pscreen, tagList);
    if (!amesa->visual)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create AROSMesaVisual\n"));
        goto error_out;
    }

    pipe = amesa->pscreen->context_create(amesa->pscreen, NULL);
    if (!pipe)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create pipe context\n"));
        goto error_out;
    }
    
    amesa->st = st_create_context(pipe, GET_GL_VIS_PTR(amesa->visual), NULL);
    if (!amesa->st)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create mesa state tracker context\n"));
        goto error_out;
    }
    
    /* Pipe context life cycle is now managed by state tracker context */
    pipe = NULL;
    
    /* Set up some needed pointers */
    /* TODO: Are those needed anymore? */
    amesa->st->ctx->DriverCtx = amesa;
    amesa->st->pipe->priv = amesa;
    
    /* Initial update of buffer dimensions (amesa->width/amesa->height) */
    AROSMesaRecalculateBufferWidthHeight(amesa);
    
    
    /* FIXME: Provide rastport to framebuffer ? */
    amesa->framebuffer = AROSMesaNewFrameBuffer(amesa, amesa->visual);
    
    return amesa;

error_out:
    if (pipe) pipe->destroy(pipe);
    if (amesa->visual) AROSMesaDestroyVisual(amesa->visual);
    if (amesa->pscreen) DestroyPipeScreen(amesa->pscreen);
    if (amesa) AROSMesaDestroyContext(amesa);
    return NULL;
}
