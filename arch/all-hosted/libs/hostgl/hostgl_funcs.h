/*
    Copyright 2011, The AROS Development Team. All rights reserved.
*/

#ifndef HOSTGL_FUNCS_H
#define HOSTGL_FUNCS_H

#include "hostgl_types.h"

BOOL HostGL_FillFBAttributes(LONG * fbattributes, LONG size, struct TagItem *tagList);
VOID HostGL_CheckAndUpdateBufferSize(struct hostgl_context *amesa);
#if defined(RENDERER_PBUFFER_WPA)
VOID HostGL_AllocatePBuffer(struct hostgl_context *amesa);
VOID HostGL_DeAllocatePBuffer(struct hostgl_context *amesa);
#endif
#if defined(RENDERER_PIXMAP_BLIT)
VOID HostGL_AllocatePixmap(struct hostgl_context *amesa);
VOID HostGL_DeAllocatePixmap(struct hostgl_context *amesa);
#endif

#endif /* HOSTGL_FUNCS_H */
