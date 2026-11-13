/* DRM stub for AROS — V3D uses v3d_ioctl_aros() instead */
#ifndef XF86DRM_H
#define XF86DRM_H

#include <stdint.h>
#include "drm.h"

#define DRM_SYNCOBJ_CREATE_SIGNALED 0x01

/* v3d_aros_override.h may have redirected this to the AROS shim; defining
 * a function of the same name would then declare that shim static. */
#ifndef drmIoctl
static inline int drmIoctl(int fd, unsigned long request, void *arg) { return -1; }
#endif
static inline int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd) { *prime_fd = -1; return -1; }
static inline int drmCloseBufferHandle(int fd, uint32_t handle) { return 0; }
static inline int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle) { return -1; }

/* Sync objects live in v3d_drm_shim.c, not as stubs: glFinish() and
 * glClientWaitSync() wait through them, from different Mesa TUs. */
extern int drmSyncobjCreate(int fd, uint32_t flags, uint32_t *handle);
extern int drmSyncobjDestroy(int fd, uint32_t handle);
extern int drmSyncobjWait(int fd, uint32_t *handles, uint32_t count,
                          int64_t timeout, uint32_t flags, uint32_t *first);
extern int drmSyncobjExportSyncFile(int fd, uint32_t handle,
                                    int *sync_file_fd);
extern int drmSyncobjImportSyncFile(int fd, uint32_t handle,
                                    int sync_file_fd);

#endif
