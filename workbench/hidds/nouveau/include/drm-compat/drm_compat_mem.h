/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_MEM_
#define _DRM_COMPAT_MEM_

#include "drm_compat_types.h"

VOID HIDDNouveauFree(APTR memory);

#define gfp_t           ULONG
#define GFP_KERNEL      (1UL < 0)
#define __GFP_ZERO      (1UL < 1)
#define GFP_DMA32       (1UL < 2)
#define GFP_HIGHUSER    (1UL < 3)
#define GFP_USER        (1UL < 4)


#define kcalloc(count, size, flags)     HIDDNouveauAlloc((count) * (size))
#define kmalloc(size, flags)            HIDDNouveauAlloc(size)
#define kzalloc(size, flags)            HIDDNouveauAlloc(size)
#define kmalloc_array(n, size, flags)   kmalloc(size *n, flags)
#define kfree(objp)                     HIDDNouveauFree((APTR)objp)

#define vmalloc_user(size)              HIDDNouveauAlloc(size)
#define vmalloc(size)                   HIDDNouveauAlloc(size)
#define vzalloc(size)                   vmalloc(size)
#define vfree(objp)                     HIDDNouveauFree(objp)

#define kvmalloc(size, flags)           HIDDNouveauAlloc(size)
#define kvcalloc(count, size, flags)    HIDDNouveauAlloc((count) * (size))
void *kvmalloc_array(size_t n, size_t size, BYTE flags);
#define kvfree(objp)                    HIDDNouveauFree(objp)

void *kmemdup(const void *src, size_t len, BYTE flags);
char *kstrndup(const char *c, size_t len, BYTE flags);
int kstrtol(const char *s, unsigned int base, long *res);

#endif /* _DRM_COMPAT_MEM_ */
