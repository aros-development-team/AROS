/*
    AROS stub for xf86drm.h

    Provides function declarations for DRM functions that Mesa's VC4
    driver uses. Implementations are in mesa3dgl's aros_drm_shim.c
    (the Mesa vc4 driver now lives in mesa3dgl.library; vc4gallium.hidd
    services the DRM ioctls via the bridge ABI).
*/

#ifndef _XF86DRM_H_AROS_
#define _XF86DRM_H_AROS_

#include <stdint.h>

/* DRM_COMMAND_BASE is defined in drm-uapi/drm.h but we need it here too */
#ifndef DRM_COMMAND_BASE
#define DRM_COMMAND_BASE 0x40
#endif

/* DRM capability constants */
#define DRM_CAP_SYNCOBJ 0x13

/* Syncobj creation flags */
#define DRM_SYNCOBJ_CREATE_SIGNALED (1 << 0)

/*
 * Implementations are in mesa3dgl-side aros_drm_shim.c. drmIoctl
 * forwards to vc4gallium via the bridge; the rest are no-op stubs.
 */
int drmIoctl(int fd, unsigned long request, void *arg);
int drmPrimeFDToHandle(int fd, int prime_fd, unsigned int *handle);
int drmPrimeHandleToFD(int fd, unsigned int handle, unsigned int flags, int *prime_fd);
int drmSyncobjCreate(int fd, unsigned int flags, unsigned int *handle);
int drmSyncobjDestroy(int fd, unsigned int handle);
int drmSyncobjImportSyncFile(int fd, unsigned int handle, int sync_file_fd);
int drmSyncobjExportSyncFile(int fd, unsigned int handle, int *sync_file_fd);

/* drmGetCap — report no capabilities on AROS */
static inline int drmGetCap(int fd, uint64_t capability, uint64_t *value)
{
    if (value)
        *value = 0;
    return -1;
}

#endif /* _XF86DRM_H_AROS_ */
