/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _DRM_MEM_UTIL_H_
#define _DRM_MEM_UTIL_H_

#include "drm_compat_funcs.h"

static inline void *drm_calloc_large(size_t nmemb, size_t size)
{
    return HIDDNouveauAlloc(nmemb * size);
}

static inline void *drm_malloc_ab(size_t nmemb, size_t size)
{
    return HIDDNouveauAlloc(nmemb * size);
}

static inline void drm_free_large(void *ptr)
{
    HIDDNouveauFree(ptr);
}

#endif
