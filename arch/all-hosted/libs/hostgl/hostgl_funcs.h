/*
    Copyright 2011-2025, The AROS Development Team. All rights reserved.
*/

#ifndef HOSTGL_FUNCS_H
#define HOSTGL_FUNCS_H

#include "hostgl_types.h"

BOOL HostGL_FillFBAttributes(LONG * fbattributes, LONG size, struct TagItem *tagList, int novisinfo);
VOID HostGL_CheckAndUpdateBufferSize(struct hostgl_context *amesa);
#if defined(RENDERER_BUFFER)
VOID HostGL_AllocateBuffer(struct hostgl_context *amesa);
VOID HostGL_DeAllocateBuffer(struct hostgl_context *amesa);
#endif

#endif /* HOSTGL_FUNCS_H */
