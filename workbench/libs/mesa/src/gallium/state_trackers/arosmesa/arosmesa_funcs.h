/*
    Copyright 2009-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_FUNCS_H
#define AROSMESA_FUNCS_H

#include AROSMESA_TYPES

VOID AROSMesaSelectRastPort(AROSMesaContext amesa, struct TagItem * tagList);
BOOL AROSMesaStandardInit(AROSMesaContext amesa, struct TagItem *tagList);
VOID AROSMesaRecalculateBufferWidthHeight(AROSMesaContext amesa);
VOID AROSMesaFreeContext(AROSMesaContext amesa);
#endif
