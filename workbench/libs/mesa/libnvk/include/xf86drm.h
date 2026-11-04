/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    libdrm's public interface, reduced to what NVK, its nouveau winsys and
    the Vulkan runtime's syncobj code use. The calls are answered by
    nvk_drm.c, which routes them to the DRM device nvgfx.hidd exports
    (hidds/nvgfx/include/libdrm/drm_entry.h); there is no file descriptor
    behind an fd here.
*/

#ifndef _XF86DRM_H_
#define _XF86DRM_H_

#include <stdint.h>
#include <sys/types.h>

#include "drm-uapi/drm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRM_DIR_NAME            "/dev/dri"
#define DRM_DEV_NAME            "%s/card%d"
#define DRM_RENDER_MINOR_NAME   "renderD"

#define DRM_NODE_PRIMARY        0
#define DRM_NODE_CONTROL        1
#define DRM_NODE_RENDER         2
#define DRM_NODE_MAX            3

#define DRM_BUS_PCI             0
#define DRM_BUS_USB             1
#define DRM_BUS_PLATFORM        2
#define DRM_BUS_HOST1X          3

#define DRM_DEVICE_GET_PCI_REVISION (1 << 0)

typedef unsigned int drm_magic_t;

typedef struct _drmVersion {
    int     version_major;
    int     version_minor;
    int     version_patchlevel;
    int     name_len;
    char    *name;
    int     date_len;
    char    *date;
    int     desc_len;
    char    *desc;
} drmVersion, *drmVersionPtr;

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

#define DRM_PLATFORM_DEVICE_NAME_LEN 512

typedef struct _drmPlatformBusInfo {
    char fullname[DRM_PLATFORM_DEVICE_NAME_LEN];
} drmPlatformBusInfo, *drmPlatformBusInfoPtr;

typedef struct _drmPlatformDeviceInfo {
    char **compatible;
} drmPlatformDeviceInfo, *drmPlatformDeviceInfoPtr;

typedef struct _drmDevice {
    char **nodes;
    int available_nodes;
    int bustype;
    union {
        drmPciBusInfoPtr pci;
        drmPlatformBusInfoPtr platform;
    } businfo;
    union {
        drmPciDeviceInfoPtr pci;
        drmPlatformDeviceInfoPtr platform;
    } deviceinfo;
} drmDevice, *drmDevicePtr;

extern int           drmOpen(const char *name, const char *busid);
extern int           drmClose(int fd);
extern drmVersionPtr drmGetVersion(int fd);
extern void          drmFreeVersion(drmVersionPtr);
extern int           drmGetCap(int fd, uint64_t capability, uint64_t *value);
extern int           drmGetMagic(int fd, drm_magic_t *magic);
extern int           drmIoctl(int fd, unsigned long request, void *arg);
extern int           drmCommandNone(int fd, unsigned long drmCommandIndex);
extern int           drmCommandRead(int fd, unsigned long drmCommandIndex,
                                    void *data, unsigned long size);
extern int           drmCommandWrite(int fd, unsigned long drmCommandIndex,
                                     void *data, unsigned long size);
extern int           drmCommandWriteRead(int fd, unsigned long drmCommandIndex,
                                         void *data, unsigned long size);

extern int           drmGetDevice2(int fd, uint32_t flags, drmDevicePtr *device);
extern int           drmGetDevices2(uint32_t flags, drmDevicePtr devices[], int max_devices);
extern int           drmGetDeviceFromDevId(dev_t find_rdev, uint32_t flags, drmDevicePtr *device);
extern void          drmFreeDevice(drmDevicePtr *device);
extern void          drmFreeDevices(drmDevicePtr devices[], int count);
extern int           drmGetNodeTypeFromFd(int fd);
extern char         *drmGetDeviceNameFromFd2(int fd);
extern char         *drmGetRenderDeviceNameFromFd(int fd);

extern int           drmCloseBufferHandle(int fd, uint32_t handle);
extern int           drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd);
extern int           drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle);

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
extern int drmSyncobjTimelineSignal(int fd, const uint32_t *handles, uint64_t *points,
                                    uint32_t handle_count);
extern int drmSyncobjTimelineWait(int fd, uint32_t *handles, uint64_t *points,
                                  unsigned num_handles, int64_t timeout_nsec, unsigned flags,
                                  uint32_t *first_signaled);
extern int drmSyncobjQuery(int fd, uint32_t *handles, uint64_t *points, uint32_t handle_count);
extern int drmSyncobjQuery2(int fd, uint32_t *handles, uint64_t *points, uint32_t handle_count,
                            uint32_t flags);
extern int drmSyncobjTransfer(int fd, uint32_t dst_handle, uint64_t dst_point,
                              uint32_t src_handle, uint64_t src_point, uint32_t flags);
extern int drmSyncobjEventfd(int fd, uint32_t handle, uint64_t point, int ev_fd, uint32_t flags);

/* AROS: a GEM object's CPU mapping, in place of mmap() on the device */
extern void *drmMMap(int fd, uint32_t handle);
extern void  drmMUnmap(int fd, uint32_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _XF86DRM_H_ */
