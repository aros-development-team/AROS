/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_FUNCS_H
#define AROSMESA_FUNCS_H

#include "arosmesa_types.h"

VOID AROSMesaSelectRastPort(AROSMesaContext amesa, struct TagItem * tagList);
BOOL AROSMesaStandardInit(AROSMesaContext amesa, struct TagItem *tagList);
BOOL AROSMesaFillVisual(struct st_visual * stvis, struct pipe_screen * screen, GLint bpp, struct TagItem *tagList);
VOID AROSMesaRecalculateBufferWidthHeight(AROSMesaContext amesa);
struct arosmesa_framebuffer * AROSMesaNewFrameBuffer(AROSMesaContext amesa, struct st_visual * stvis);
VOID AROSMesaDestroyContext(AROSMesaContext amesa);
VOID AROSMesaDestroyFrameBuffer(struct arosmesa_framebuffer * framebuffer);
VOID AROSMesaCheckAndUpdateBufferSize(AROSMesaContext amesa);
struct st_manager * AROSMesaNewStManager();
VOID AROSMesaDestroyStManager(struct st_manager * stmanager);
#endif
