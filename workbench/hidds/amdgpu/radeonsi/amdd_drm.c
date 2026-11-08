/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    radeonsi.amdd's libdrm: the drm* calls its copy of libdrm_amdgpu
    (libdrm/amdgpu) and Mesa's amdgpu winsys make, answered through the
    driver's DRM entry table (libdrm/drm_entry.h). Each drmOpen() is a
    DRM client of its own. Return values follow the driver's in-process
    libdrm (libdrm/arosdrm.c), which both are written against: a negative
    errno on failure.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <errno.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xf86drm.h>
#include <libdrm/drm_entry.h>

#include "amdd_gallium.h"

const struct amdgpu_drm_entry *amdd_drm;

int drmOpen(const char *name, const char *busid)
{
    (void)name; (void)busid;
    if (!amdd_drm)
        return -ENODEV;
    return amdd_drm->open();
}

int drmClose(int fd)
{
    if (!amdd_drm)
        return -ENODEV;
    return amdd_drm->close(fd);
}

int drmDupFd(int fd)
{
    if (!amdd_drm || amdd_drm->size < offsetof(struct amdgpu_drm_entry, dup) + sizeof(amdd_drm->dup)
        || !amdd_drm->dup)
        return -ENOSYS;
    return amdd_drm->dup(fd);
}

int drmIoctl(int fd, unsigned long request, void *arg)
{
    if (!amdd_drm)
        return -ENODEV;
    return amdd_drm->ioctl(fd, request, arg);
}

int drmCommandNone(int fd, unsigned long index)
{
    if (!amdd_drm)
        return -ENODEV;
    return amdd_drm->command(fd, index, NULL, 0);
}

int drmCommandRead(int fd, unsigned long index, void *data, unsigned long size)
{
    if (!amdd_drm)
        return -ENODEV;
    return amdd_drm->command(fd, index, data, size);
}

int drmCommandWrite(int fd, unsigned long index, void *data, unsigned long size)
{
    if (!amdd_drm)
        return -ENODEV;
    return amdd_drm->command(fd, index, data, size);
}

int drmCommandWriteRead(int fd, unsigned long index, void *data, unsigned long size)
{
    if (!amdd_drm)
        return -ENODEV;
    return amdd_drm->command(fd, index, data, size);
}

drmVersionPtr drmGetVersion(int fd)
{
    struct { drmVersion v; char name[64]; } *ver;

    if (!amdd_drm)
        return NULL;
    ver = calloc(1, sizeof(*ver));
    if (!ver)
        return NULL;
    if (amdd_drm->get_version(fd, &ver->v.version_major, &ver->v.version_minor,
                              &ver->v.version_patchlevel, ver->name, sizeof(ver->name)))
    {
        free(ver);
        return NULL;
    }
    ver->v.name = ver->name;
    ver->v.name_len = strlen(ver->name);
    ver->v.date = ver->v.desc = (char *)"";
    return &ver->v;
}

void drmFreeVersion(drmVersionPtr ver)
{
    free(ver);
}

int drmGetDevice2(int fd, uint32_t flags, drmDevicePtr *device)
{
    struct amdgpu_drm_pci_info info;
    struct {
        drmDevice dev;
        drmPciBusInfo bus;
        drmPciDeviceInfo pci;
        char *nodes[DRM_NODE_MAX];
        char primary[16];
    } *d;

    (void)flags;
    if (!amdd_drm || amdd_drm->get_pci_info(fd, &info))
        return -ENODEV;
    d = calloc(1, sizeof(*d));
    if (!d)
        return -ENOMEM;
    d->dev.bustype = DRM_BUS_PCI;
    d->dev.businfo.pci = &d->bus;
    d->dev.deviceinfo.pci = &d->pci;
    d->dev.nodes = d->nodes;
    strcpy(d->primary, "/dev/dri/card0");
    d->nodes[DRM_NODE_PRIMARY] = d->primary;
    d->dev.available_nodes = 1 << DRM_NODE_PRIMARY;
    d->bus.domain = info.domain;
    d->bus.bus = info.bus;
    d->bus.dev = info.dev;
    d->bus.func = info.func;
    d->pci.vendor_id = info.vendor_id;
    d->pci.device_id = info.device_id;
    d->pci.subvendor_id = info.subvendor_id;
    d->pci.subdevice_id = info.subdevice_id;
    d->pci.revision_id = info.revision_id;
    *device = &d->dev;
    return 0;
}

void drmFreeDevice(drmDevicePtr *device)
{
    if (device && *device)
    {
        free(*device);
        *device = NULL;
    }
}

void *drmMMap(int fd, uint32_t handle, VOID (*unmapped)(APTR), APTR data)
{
    (void)unmapped; (void)data;
    if (!amdd_drm)
        return NULL;
    return amdd_drm->map(fd, handle);
}

void drmMUnmap(int fd, uint32_t handle)
{
    if (amdd_drm)
        amdd_drm->unmap(fd, handle);
}

int drmCloseBufferHandle(int fd, uint32_t handle)
{
    struct drm_gem_close args = { .handle = handle };

    return drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &args);
}

int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd)
{
    return -ENOSYS;
}

int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle)
{
    return -ENOSYS;
}

int drmGetCap(int fd, uint64_t capability, uint64_t *value)
{
    struct drm_get_cap req = { .capability = capability };
    int ret = drmIoctl(fd, DRM_IOCTL_GET_CAP, &req);

    if (ret == 0)
        *value = req.value;
    return ret;
}

int drmGetNodeTypeFromFd(int fd)
{
    return DRM_NODE_PRIMARY;
}

char *drmGetPrimaryDeviceNameFromFd(int fd)
{
    char *name = malloc(16);

    if (name)
        strcpy(name, "/dev/dri/card0");
    return name;
}

/* No render nodes on AROS; the amdgpu winsys only asks for one after a
 * failed EACCES open of a primary node, which cannot happen here either. */
char *drmGetRenderDeviceNameFromFd(int fd)
{
    return NULL;
}

char *drmGetFormatModifierName(uint64_t modifier)
{
    return NULL;
}

void drmMsg(const char *format, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    amdd_log("[libdrm] %s", buf);
}

int drmSyncobjCreate(int fd, uint32_t flags, uint32_t *handle)
{
    struct drm_syncobj_create args = { .flags = flags };
    int ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_CREATE, &args);

    if (ret == 0)
        *handle = args.handle;
    return ret;
}

int drmSyncobjDestroy(int fd, uint32_t handle)
{
    struct drm_syncobj_destroy args = { .handle = handle };

    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_DESTROY, &args);
}

int drmSyncobjHandleToFD(int fd, uint32_t handle, int *obj_fd)
{
    return -ENOSYS;
}

int drmSyncobjFDToHandle(int fd, int obj_fd, uint32_t *handle)
{
    return -ENOSYS;
}

int drmSyncobjImportSyncFile(int fd, uint32_t handle, int sync_file_fd)
{
    return -ENOSYS;
}

int drmSyncobjExportSyncFile(int fd, uint32_t handle, int *sync_file_fd)
{
    return -ENOSYS;
}

int drmSyncobjWait(int fd, uint32_t *handles, unsigned num_handles,
                   int64_t timeout_nsec, unsigned flags, uint32_t *first_signaled)
{
    struct drm_syncobj_wait args = {
        .handles = (uintptr_t)handles,
        .timeout_nsec = timeout_nsec,
        .count_handles = num_handles,
        .flags = flags,
    };
    int ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_WAIT, &args);

    if (ret == 0 && first_signaled)
        *first_signaled = args.first_signaled;
    return ret;
}

int drmSyncobjReset(int fd, const uint32_t *handles, uint32_t handle_count)
{
    struct drm_syncobj_array args = { .handles = (uintptr_t)handles, .count_handles = handle_count };

    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_RESET, &args);
}

int drmSyncobjSignal(int fd, const uint32_t *handles, uint32_t handle_count)
{
    struct drm_syncobj_array args = { .handles = (uintptr_t)handles, .count_handles = handle_count };

    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_SIGNAL, &args);
}

int drmSyncobjTimelineSignal(int fd, const uint32_t *handles, uint64_t *points, uint32_t handle_count)
{
    struct drm_syncobj_timeline_array args = {
        .handles = (uintptr_t)handles, .points = (uintptr_t)points, .count_handles = handle_count,
    };

    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_TIMELINE_SIGNAL, &args);
}

int drmSyncobjTimelineWait(int fd, uint32_t *handles, uint64_t *points, unsigned num_handles,
                           int64_t timeout_nsec, unsigned flags, uint32_t *first_signaled)
{
    struct drm_syncobj_timeline_wait args = {
        .handles = (uintptr_t)handles, .points = (uintptr_t)points,
        .timeout_nsec = timeout_nsec, .count_handles = num_handles, .flags = flags,
    };
    int ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_TIMELINE_WAIT, &args);

    if (ret == 0 && first_signaled)
        *first_signaled = args.first_signaled;
    return ret;
}

int drmSyncobjQuery2(int fd, uint32_t *handles, uint64_t *points, uint32_t handle_count, uint32_t flags)
{
    struct drm_syncobj_timeline_array args = {
        .handles = (uintptr_t)handles, .points = (uintptr_t)points,
        .count_handles = handle_count, .flags = flags,
    };

    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_QUERY, &args);
}

int drmSyncobjQuery(int fd, uint32_t *handles, uint64_t *points, uint32_t handle_count)
{
    return drmSyncobjQuery2(fd, handles, points, handle_count, 0);
}

int drmSyncobjTransfer(int fd, uint32_t dst_handle, uint64_t dst_point,
                       uint32_t src_handle, uint64_t src_point, uint32_t flags)
{
    struct drm_syncobj_transfer args = {
        .src_handle = src_handle, .dst_handle = dst_handle,
        .src_point = src_point, .dst_point = dst_point, .flags = flags,
    };

    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_TRANSFER, &args);
}
