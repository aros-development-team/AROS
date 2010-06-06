/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: aros_fb_functions.c 31737 2009-08-23 13:34:05Z deadwood $
*/

#include "aros_fb_functions.h"
#include "aros_rb_functions.h"


#include <aros/debug.h>
#include "framebuffer.h"

static void
_aros_destroy_framebuffer(struct gl_framebuffer * fb)
{
    D(bug("[AROSMESA] _aros_destroy_framebuffer\n"));

    if (fb)
    {
        _mesa_free_framebuffer_data(fb);
        FreeVec(GET_AROS_FB_PTR(fb));
    }
}


AROSMesaFrameBuffer aros_new_framebuffer(GLvisual * visual)
{
    AROSMesaFrameBuffer aros_fb = NULL;
    struct gl_framebuffer * fb = NULL;

    D(bug("[AROSMESA] aros_new_framebuffer\n"));

    /* Allocated memory for aros structure */
    aros_fb = AllocVec(sizeof(struct arosmesa_framebuffer), MEMF_PUBLIC | MEMF_CLEAR);

    if (!aros_fb)
        return NULL;

    fb = GET_GL_FB_PTR(aros_fb);

    /* Initialize mesa structure */
    _mesa_initialize_framebuffer(fb, visual);

    fb->Delete = _aros_destroy_framebuffer;
    
    return aros_fb;
}

