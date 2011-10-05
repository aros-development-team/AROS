/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#if !defined(DRM_REDEFINES_H)
#define DRM_REDEFINES_H

/* THESE REDEFINES ARE NECESSARY AS WE BUILD USER AND KERNEL INTO ONE MODULE
   AND THEY HAVE OVERLAPPING FUNCTION NAMES! */

#define nouveau_notifier_alloc              _redefined_nouveau_notifier_alloc
#define nouveau_channel_alloc               _redefined_nouveau_channel_alloc
#define nouveau_channel_free                _redefined_nouveau_channel_free


#define nouveau_bo_new                      _redefined_nouveau_bo_new
#define nouveau_bo_pin                      _redefined_nouveau_bo_pin
#define nouveau_bo_unpin                    _redefined_nouveau_bo_unpin
#define nouveau_bo_map                      _redefined_nouveau_bo_map
#define nouveau_bo_unmap                    _redefined_nouveau_bo_unmap

#define nouveau_fence_new                   _redefined_nouveau_fence_new
#define nouveau_fence_emit                  _redefined_nouveau_fence_emit
#define nouveau_fence_work                  _redefined_nouveau_fence_work
#define nouveau_fence_update                _redefined_nouveau_fence_update
#endif

