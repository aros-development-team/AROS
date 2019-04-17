/*
    Copyright 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef MESA3DGL_GALLIUM_H
#define MESA3DGL_GALLIUM_H

#include "mesa3dgl_types.h"

BOOL MESA3DGLFillVisual(struct st_visual * stvis, struct pipe_screen * screen, GLint bpp, struct TagItem *tagList);
struct mesa3dgl_framebuffer * MESA3DGLNewFrameBuffer(struct mesa3dgl_context * ctx, struct st_visual * stvis);
VOID MESA3DGLFreeFrameBuffer(struct mesa3dgl_framebuffer * framebuffer);
VOID MESA3DGLCheckAndUpdateBufferSize(struct mesa3dgl_context * ctx);
struct st_manager * MESA3DGLNewStManager();
VOID MESA3DGLFreeStManager(APTR pipe, struct st_manager * stmanager);

#endif /* MESA3DGL_GALLIUM_H */
