/* DRM stub for AROS — V3D uses v3d_ioctl_aros() instead */
#ifndef XF86DRM_H
#define XF86DRM_H

#include <stdint.h>

#define DRM_SYNCOBJ_CREATE_SIGNALED 0x01
#define DRM_COMMAND_BASE 0x40

#define DRM_IOWR(nr, type) (nr)
#define DRM_IOW(nr, type) (nr)

static inline int drmIoctl(int fd, unsigned long request, void *arg) { return -1; }
static inline int drmSyncobjCreate(int fd, uint32_t flags, uint32_t *handle) { *handle = 1; return 0; }
static inline int drmSyncobjWait(int fd, uint32_t *handles, uint32_t count, int64_t timeout, uint32_t flags, uint32_t *first) { return 0; }
static inline int drmSyncobjDestroy(int fd, uint32_t handle) { return 0; }
static inline int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd) { *prime_fd = -1; return -1; }
static inline int drmCloseBufferHandle(int fd, uint32_t handle) { return 0; }

#endif
