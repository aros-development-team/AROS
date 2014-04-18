/*
    Copyright 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_FUNCS_H
#define AROSMESA_FUNCS_H

#include AROSMESA_TYPES

VOID AROSMesaSelectRastPort(struct arosmesa_context * amesa, struct TagItem * tagList);
BOOL AROSMesaStandardInit(struct arosmesa_context * amesa, struct TagItem *tagList);
VOID AROSMesaRecalculateBufferWidthHeight(struct arosmesa_context * amesa);
VOID AROSMesaFreeContext(struct arosmesa_context * amesa);
#endif
