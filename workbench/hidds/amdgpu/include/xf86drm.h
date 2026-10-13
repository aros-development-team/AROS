/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _XF86DRM_H_
#define _XF86DRM_H_

/* libdrm's public interface, reduced to what the amdgpu winsys and
   libdrm_amdgpu use; the calls are answered by the in-process shim in
   libdrm/arosdrm.c */
#include <libdrm/arosdrm.h>

#include <stdint.h>
#include <sys/types.h>

#define DRM_DIR_NAME            "/dev/dri"
#define DRM_DEV_NAME            "%s/card%d"
#define DRM_RENDER_MINOR_NAME   "renderD"

/* the prime flags come from uapi/drm/drm.h, via arosdrm.h */

extern int drmCloseBufferHandle(int fd, uint32_t handle);
extern int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd);
extern int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle);

#define DRM_NODE_PRIMARY        0
#define DRM_NODE_CONTROL        1
#define DRM_NODE_RENDER         2
#define DRM_NODE_MAX            3

#define DRM_BUS_PCI             0

typedef struct _drmPciBusInfo {
    uint16_t domain;
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
} drmPciBusInfo, *drmPciBusInfoPtr;

typedef struct _drmPciDeviceInfo {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t subvendor_id;
    uint16_t subdevice_id;
    uint8_t revision_id;
} drmPciDeviceInfo, *drmPciDeviceInfoPtr;

typedef struct _drmDevice {
    char **nodes;
    int available_nodes;
    int bustype;
    union {
        drmPciBusInfoPtr pci;
    } businfo;
    union {
        drmPciDeviceInfoPtr pci;
    } deviceinfo;
} drmDevice, *drmDevicePtr;

extern int drmGetCap(int fd, uint64_t capability, uint64_t *value);
extern int drmGetDevice2(int fd, uint32_t flags, drmDevicePtr *device);
extern void drmFreeDevice(drmDevicePtr *device);
extern int drmGetNodeTypeFromFd(int fd);
extern char *drmGetPrimaryDeviceNameFromFd(int fd);
extern char *drmGetFormatModifierName(uint64_t modifier);
extern void drmMsg(const char *format, ...);

extern int drmSyncobjCreate(int fd, uint32_t flags, uint32_t *handle);
extern int drmSyncobjDestroy(int fd, uint32_t handle);
extern int drmSyncobjHandleToFD(int fd, uint32_t handle, int *obj_fd);
extern int drmSyncobjFDToHandle(int fd, int obj_fd, uint32_t *handle);
extern int drmSyncobjImportSyncFile(int fd, uint32_t handle, int sync_file_fd);
extern int drmSyncobjExportSyncFile(int fd, uint32_t handle, int *sync_file_fd);
extern int drmSyncobjWait(int fd, uint32_t *handles, unsigned num_handles,
                          int64_t timeout_nsec, unsigned flags, uint32_t *first_signaled);
extern int drmSyncobjReset(int fd, const uint32_t *handles, uint32_t handle_count);
extern int drmSyncobjSignal(int fd, const uint32_t *handles, uint32_t handle_count);
extern int drmSyncobjTimelineSignal(int fd, const uint32_t *handles, uint64_t *points, uint32_t handle_count);
extern int drmSyncobjTimelineWait(int fd, uint32_t *handles, uint64_t *points, unsigned num_handles,
                                  int64_t timeout_nsec, unsigned flags, uint32_t *first_signaled);
extern int drmSyncobjQuery(int fd, uint32_t *handles, uint64_t *points, uint32_t handle_count);
extern int drmSyncobjQuery2(int fd, uint32_t *handles, uint64_t *points, uint32_t handle_count, uint32_t flags);
extern int drmSyncobjTransfer(int fd, uint32_t dst_handle, uint64_t dst_point,
                              uint32_t src_handle, uint64_t src_point, uint32_t flags);

#endif
