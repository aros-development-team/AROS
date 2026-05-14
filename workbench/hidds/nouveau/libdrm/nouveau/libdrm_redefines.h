/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#if !defined(LIBDRM_REDEFINES_H)
#define LIBDRM_REDEFINES_H

/* THESE REDEFINES ARE NECESSARY AS WE BUILD USER AND KERNEL INTO ONE MODULE
   AND THEY HAVE OVERLAPPING FUNCTION NAMES! */

#define nouveau_notifier_alloc              libdrm_nouveau_notifier_alloc
#define nouveau_channel_alloc               libdrm_nouveau_channel_alloc
#define nouveau_channel_free                libdrm_nouveau_channel_free


#define nouveau_bo_init                     libdrm_nouveau_bo_init
#define nouveau_bo_new                      libdrm_nouveau_bo_new
#define nouveau_bo_pin                      libdrm_nouveau_bo_pin
#define nouveau_bo_unpin                    libdrm_nouveau_bo_unpin
#define nouveau_bo_map                      libdrm_nouveau_bo_map
#define nouveau_bo_unmap                    libdrm_nouveau_bo_unmap

#define nouveau_fence_new                   libdrm_nouveau_fence_new
#define nouveau_fence_emit                  libdrm_nouveau_fence_emit
#define nouveau_fence_work                  libdrm_nouveau_fence_work
#define nouveau_fence_update                libdrm_nouveau_fence_update
#endif

