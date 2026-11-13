/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    nouveau.nvdd's libdrm: the drm* calls its copy of libdrm_nouveau
    (libdrm/nouveau/nouveau.c) makes, answered through the driver's DRM
    entry table (libdrm/drm_entry.h). Each drmOpen() is a DRM client of
    its own. Return values follow the driver's in-process libdrm
    (libdrm/arosdrm.c), which libdrm_nouveau is written against: a
    negative errno on failure.
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
#include <uapi/drm/drm.h>

#include "libdrm/drm_entry.h"
#include "nvdd_gallium.h"

const struct nouveau_drm_entry *nvdd_drm;
void (*nvdd_log)(const char *fmt, ...);

#define ENTRY_HAS(field) \
    (nvdd_drm && nvdd_drm->size >= offsetof(struct nouveau_drm_entry, field) + sizeof(nvdd_drm->field) \
     && nvdd_drm->field)

int drmOpen(const char *name, const char *busid)
{
    (void)name; (void)busid;
    if (!nvdd_drm)
        return -ENODEV;
    return nvdd_drm->open();
}

int drmClose(int fd)
{
    if (!nvdd_drm)
        return -ENODEV;
    return nvdd_drm->close(fd);
}

int drmIoctl(int fd, unsigned long request, void *arg)
{
    if (!nvdd_drm)
        return -ENODEV;
    return nvdd_drm->ioctl(fd, request, arg);
}

int drmCommandWrite(int fd, unsigned long index, void *data, unsigned long size)
{
    if (!nvdd_drm)
        return -ENODEV;
    return nvdd_drm->command(fd, index, data, size);
}

int drmCommandWriteRead(int fd, unsigned long index, void *data, unsigned long size)
{
    if (!nvdd_drm)
        return -ENODEV;
    return nvdd_drm->command(fd, index, data, size);
}

drmVersionPtr drmGetVersion(int fd)
{
    struct { drmVersion v; char name[64]; } *ver;

    if (!nvdd_drm)
        return NULL;
    ver = calloc(1, sizeof(*ver));
    if (!ver)
        return NULL;
    if (nvdd_drm->get_version(fd, &ver->v.version_major, &ver->v.version_minor,
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
    struct nouveau_drm_pci_info info;
    struct {
        drmDevice dev;
        drmPciBusInfo bus;
        drmPciDeviceInfo pci;
    } *d;

    (void)flags;
    if (!nvdd_drm || nvdd_drm->get_pci_info(fd, &info))
        return -ENODEV;
    d = calloc(1, sizeof(*d));
    if (!d)
        return -ENOMEM;
    d->dev.bustype = DRM_BUS_PCI;
    d->dev.businfo.pci = &d->bus;
    d->dev.deviceinfo.pci = &d->pci;
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
    if (!nvdd_drm)
        return NULL;
    /* libdrm_nouveau needs to hear when the kernel moves the object */
    if (ENTRY_HAS(map_notify))
        return nvdd_drm->map_notify(fd, handle, (void (*)(void *))unmapped, data);
    return nvdd_drm->map(fd, handle);
}

void drmMUnmap(int fd, uint32_t handle)
{
    if (nvdd_drm)
        nvdd_drm->unmap(fd, handle);
}

int drmCloseBufferHandle(int fd, uint32_t handle)
{
    struct drm_gem_close args = { .handle = handle };

    return drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &args);
}

/* dma-buf export/import: not used by the gallium driver on AROS (buffers
   are shared by GEM name) */
int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd)
{
    (void)fd; (void)handle; (void)flags; (void)prime_fd;
    return -ENOSYS;
}

int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle)
{
    (void)fd; (void)prime_fd; (void)handle;
    return -ENOSYS;
}

/* libdrm_nouveau's log, to the driver's */
void nouveau_compat_log(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    if (!nvdd_log)
        return;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    nvdd_log("%s", buf);
}
