/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_FUNCS_H
#define AROSMESA_FUNCS_H

#include "arosmesa_types.h"

VOID AROSMesaSelectRastPort(AROSMesaContext amesa, struct TagItem * tagList);
BOOL AROSMesaStandardInit(AROSMesaContext amesa, struct TagItem *tagList);
AROSMesaVisual AROSMesaNewVisual(GLint bpp, struct pipe_screen * screen, struct TagItem *tagList);
GLboolean AROSMesaRecalculateBufferWidthHeight(AROSMesaContext amesa);
AROSMesaFrameBuffer AROSMesaNewFrameBuffer(AROSMesaContext amesa, AROSMesaVisual visual);
VOID AROSMesaDestroyContext(AROSMesaContext amesa);
VOID AROSMesaDestroyVisual(AROSMesaVisual aros_vis);
VOID AROSMesaDestroyFrameBuffer(AROSMesaFrameBuffer aros_fb);
VOID AROSMesaCheckAndUpdateBufferSize(AROSMesaContext amesa);

#endif
