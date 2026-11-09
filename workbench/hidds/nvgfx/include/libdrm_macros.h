/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LIBDRM_MACROS_H_
#define _LIBDRM_MACROS_H_

#define drm_private
#define drm_public
#define drm_mmap(addr, length, prot, flags, fd, offset)  ((void *)-1)
#define drm_munmap(addr, length)                         (0)
#define MAP_FAILED                                       ((void *)-1)
#define PROT_READ   1
#define PROT_WRITE  2
#define MAP_SHARED  1

#endif
