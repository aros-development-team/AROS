/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: aros_fb_functions.h 31737 2009-08-23 13:34:05Z deadwood $
*/

#if !defined(AROS_FB_FUNCTIONS_H)
#define AROS_FB_FUNCTIONS_H

#include <GL/gl.h>
#include "context.h"
#include "arosmesa_internal.h"

AROSMesaFrameBuffer aros_new_framebuffer(GLvisual * visual);

#endif /* AROS_FB_FUNCTIONS_H */
