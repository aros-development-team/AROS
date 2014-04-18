/*
    Copyright 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_FUNCS_GALLIUM_H
#define AROSMESA_FUNCS_GALLIUM_H

#include "arosmesa_types.h"

BOOL AROSMesaFillVisual(struct st_visual * stvis, struct pipe_screen * screen, GLint bpp, struct TagItem *tagList);
struct arosmesa_framebuffer * AROSMesaNewFrameBuffer(struct arosmesa_context * amesa, struct st_visual * stvis);
VOID AROSMesaFreeFrameBuffer(struct arosmesa_framebuffer * framebuffer);
VOID AROSMesaCheckAndUpdateBufferSize(struct arosmesa_context * amesa);
struct st_manager * AROSMesaNewStManager();
VOID AROSMesaFreeStManager(struct st_manager * stmanager);
#endif
