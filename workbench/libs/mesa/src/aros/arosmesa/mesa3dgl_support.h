/*
    Copyright 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef MESA3DGL_SUPPORT_H
#define MESA3DGL_SUPPORT_H

#include "mesa3dgl_types.h"

VOID MESA3DGLSelectRastPort(struct mesa3dgl_context * amesa, struct TagItem * tagList);
BOOL MESA3DGLStandardInit(struct mesa3dgl_context * amesa, struct TagItem *tagList);
VOID MESA3DGLRecalculateBufferWidthHeight(struct mesa3dgl_context * amesa);
VOID MESA3DGLFreeContext(struct mesa3dgl_context * amesa);

#endif /* MESA3DGL_SUPPORT_H */
