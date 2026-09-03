/*
    Copyright 2010-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <libdrm/arosdrm.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>

#include <drm/drm_drv.h>
#include <drm/drm_gem.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <uapi/drm/nouveau_drm.h>

#include "drm_crtc_internal.h"
#include "drm_internal.h"
#include <xf86drm.h>
#include <linux/pci.h>

/*
 * libdrm expects a file descriptor and ioctls; here the "kernel" is the same
 * binary, so an fd is an index into a table of drm_files and an ioctl is a
 * direct call into the driver's dispatch table.
 */

void nouveau_compat_log(const char *fmt, ...);
void *drm_gem_nouveau_mmap(struct drm_device *dev, struct drm_file *f, uint32_t handle, VOID (*unmapped)(APTR), APTR data);
void drm_gem_nouveau_munmap(struct drm_device *dev, struct drm_file *f, uint32_t handle);
BOOL drm_nouveau_get_monitor_name(struct drm_device *dev, uint32_t connector_id, char *name, int namelen);
int nouveau_abi16_ioctl(struct drm_file *filp, void __user *user, u32 size);

extern struct drm_device *current_drm_device;

#define MAX_DRM_FILES 128
static struct drm_file *drm_files[MAX_DRM_FILES] = { NULL };

static int drm_driver_ioctl(int fd, unsigned long index, void *data, unsigned long size)
{
    struct drm_device *dev = current_drm_device;
    const struct drm_driver *drv;
    const struct drm_ioctl_desc *desc;

    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd] || !dev)
        return -EINVAL;

    drv = dev->driver;
    if (!drv || !drv->ioctls)
        return -EINVAL;

    /* the NVIF channel is dispatched ahead of the table in the driver too */
    if (index == DRM_NOUVEAU_NVIF)
        return nouveau_abi16_ioctl(drm_files[fd], data, size);

    if (index >= drv->num_ioctls)
        return -EINVAL;
    desc = &drv->ioctls[index];
    if (!desc->func)
        return -EINVAL;

    return desc->func(dev, data, drm_files[fd]);
}

int drmCommandNone(int fd, unsigned long drmCommandIndex)
{
    return drm_driver_ioctl(fd, drmCommandIndex, NULL, 0);
}

int drmCommandRead(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
    return drm_driver_ioctl(fd, drmCommandIndex, data, size);
}

int drmCommandWrite(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
    return drm_driver_ioctl(fd, drmCommandIndex, data, size);
}

int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
    return drm_driver_ioctl(fd, drmCommandIndex, data, size);
}

int drmOpen(const char *name, const char *busid)
{
    int i;

    if (!current_drm_device)
        return -ENODEV;

    for (i = 0; i < MAX_DRM_FILES; i++)
    {
        if (drm_files[i] == NULL)
        {
            struct drm_file *file = drm_file_alloc(current_drm_device->primary);
            if (IS_ERR(file))
                return PTR_ERR(file);
            drm_files[i] = file;
            return i;
        }
    }

    return -EMFILE;
}

int drmClose(int fd)
{
    struct drm_file *f;

    if (fd < 0 || fd >= MAX_DRM_FILES || !(f = drm_files[fd]))
        return 0;

    drm_files[fd] = NULL;
    drm_file_free(f);
    return 0;
}

drmVersionPtr drmGetVersion(int fd)
{
    static drmVersion ver;

    if (current_drm_device && current_drm_device->driver)
    {
        ver.version_major = current_drm_device->driver->major;
        ver.version_minor = current_drm_device->driver->minor;
        ver.version_patchlevel = current_drm_device->driver->patchlevel;
        ver.name = (char *)current_drm_device->driver->name;
        ver.name_len = strlen(ver.name);
    }
    else
    {
        memset(&ver, 0, sizeof(ver));
    }

    return &ver;
}

void drmFreeVersion(drmVersionPtr ptr)
{
}

int drmCreateContext(int fd, drm_context_t *handle)
{
    return 0;
}

int drmDestroyContext(int fd, drm_context_t handle)
{
    return 0;
}

int drmIoctl(int fd, unsigned long request, void *arg)
{
    struct drm_device *dev = current_drm_device;
    struct drm_file *file;
    int ret = -EINVAL;

    if (fd < 0 || fd >= MAX_DRM_FILES || !(file = drm_files[fd]))
        return ret;

    do
    {
        switch (request)
        {
            case DRM_IOCTL_GEM_CLOSE:       ret = drm_gem_close_ioctl(dev, arg, file); break;
            case DRM_IOCTL_GEM_OPEN:        ret = drm_gem_open_ioctl(dev, arg, file); break;
            case DRM_IOCTL_GEM_FLINK:       ret = drm_gem_flink_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_ADDFB:      ret = drm_mode_addfb_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_ADDFB2:     ret = drm_mode_addfb2_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_RMFB:       ret = drm_mode_rmfb_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETFB:      ret = drm_mode_getfb(dev, arg, file); break;
            case DRM_IOCTL_MODE_SETCRTC:    ret = drm_mode_setcrtc(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETCRTC:    ret = drm_mode_getcrtc(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETRESOURCES: ret = drm_mode_getresources(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETCONNECTOR: ret = drm_mode_getconnector(dev, arg, file); break;
            case DRM_IOCTL_MODE_CURSOR:     ret = drm_mode_cursor_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_CURSOR2:    ret = drm_mode_cursor2_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETENCODER: ret = drm_mode_getencoder(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETPLANERESOURCES: ret = drm_mode_getplane_res(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETPLANE:   ret = drm_mode_getplane(dev, arg, file); break;
            case DRM_IOCTL_MODE_SETPLANE:   ret = drm_mode_setplane(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETPROPERTY: ret = drm_mode_getproperty_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETPROPBLOB: ret = drm_mode_getblob_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_OBJ_GETPROPERTIES: ret = drm_mode_obj_get_properties_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_OBJ_SETPROPERTY: ret = drm_mode_obj_set_property_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_ATOMIC:     ret = drm_mode_atomic_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETGAMMA:   ret = drm_mode_gamma_get_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_SETGAMMA:   ret = drm_mode_gamma_set_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_PAGE_FLIP:  ret = drm_mode_page_flip_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_DIRTYFB:    ret = drm_mode_dirtyfb_ioctl(dev, arg, file); break;
            default:
                bug("[nouveau] drmIoctl: request 0x%lx not implemented\n", request);
                return -EINVAL;
        }
    } while (ret == -EINTR || ret == -EAGAIN);

    if (ret)
        nouveau_compat_log("[nouveau] drmIoctl: request 0x%lx failed, %d\n", request, ret);

    return ret;
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

void *drmMalloc(int size)
{
    return kzalloc(size, GFP_KERNEL);
}

void drmFree(void *pt)
{
    kfree(pt);
}

/* The card the driver probed; drm-aros records it during bring-up. */
extern struct pci_dev *nouveau_aros_pdev;

int drmGetDevice2(int fd, uint32_t flags, drmDevicePtr *device)
{
    struct pci_dev *pdev = nouveau_aros_pdev;
    struct {
        drmDevice dev;
        drmPciBusInfo bus;
        drmPciDeviceInfo info;
    } *d;

    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd] || !pdev)
        return -ENODEV;

    d = kzalloc(sizeof(*d), GFP_KERNEL);
    if (!d)
        return -ENOMEM;

    d->dev.bustype = DRM_BUS_PCI;
    d->dev.businfo.pci = &d->bus;
    d->dev.deviceinfo.pci = &d->info;
    d->bus.domain = 0;
    d->bus.bus = pdev->bus ? pdev->bus->number : 0;
    d->bus.dev = PCI_SLOT(pdev->devfn);
    d->bus.func = PCI_FUNC(pdev->devfn);
    d->info.vendor_id = pdev->vendor;
    d->info.device_id = pdev->device;
    d->info.subvendor_id = pdev->subsystem_vendor;
    d->info.subdevice_id = pdev->subsystem_device;
    d->info.revision_id = pdev->revision;

    *device = &d->dev;
    return 0;
}

void drmFreeDevice(drmDevicePtr *device)
{
    if (device && *device)
    {
        kfree(*device);
        *device = NULL;
    }
}

void *drmMMap(int fd, uint32_t handle, VOID (*unmapped)(APTR), APTR data)
{
    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd])
        return NULL;
    return drm_gem_nouveau_mmap(current_drm_device, drm_files[fd], handle, unmapped, data);
}

void drmMUnmap(int fd, uint32_t handle)
{
    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd])
        return;
    drm_gem_nouveau_munmap(current_drm_device, drm_files[fd], handle);
}

BOOL drm_nouveau_get_chip_name(struct drm_device *dev, char *name, int namelen);

BOOL drmGetChipName(int fd, char *name, int namelen)
{
    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd])
        return FALSE;

    return drm_nouveau_get_chip_name(current_drm_device, name, namelen);
}

BOOL drmGetMonitorName(int fd, uint32_t connector_id, char *name, int namelen)
{
    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd])
        return FALSE;

    return drm_nouveau_get_monitor_name(current_drm_device, connector_id, name, namelen);
}
