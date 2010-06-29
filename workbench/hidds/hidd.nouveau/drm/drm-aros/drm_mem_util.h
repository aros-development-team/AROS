/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _DRM_MEM_UTIL_H_
#define _DRM_MEM_UTIL_H_

#include <proto/exec.h>

static __inline__ void *drm_calloc_large(size_t nmemb, size_t size)
{
    return AllocVec(nmemb * size, MEMF_ANY | MEMF_CLEAR);
}

static __inline__ void *drm_malloc_ab(size_t nmemb, size_t size)
{
    return AllocVec(nmemb * size, MEMF_ANY);
}

static __inline void drm_free_large(void *ptr)
{
    FreeVec(ptr);
}

#endif
