/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _XF86DRM_H_
#define _XF86DRM_H_

/* libdrm's public interface, reduced to what the nouveau library uses;
   the calls are answered by the in-process shim in libdrm/arosdrm.c */
#include <libdrm/arosdrm.h>

#include <stdint.h>
#include <sys/types.h>

#define DRM_DIR_NAME            "/dev/dri"
#define DRM_DEV_NAME            "%s/card%d"
#define DRM_RENDER_MINOR_NAME   "renderD"

extern int drmCloseBufferHandle(int fd, uint32_t handle);
extern int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd);
extern int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle);

#endif
