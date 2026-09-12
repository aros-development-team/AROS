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

#include "drm_crtc_internal.h"
#include "drm_internal.h"
#include <xf86drm.h>
#include <linux/pci.h>

/*
 * libdrm expects a file descriptor and ioctls; here the "kernel" is the same
 * binary, so an fd is an index into a table of drm_files and an ioctl is a
 * direct call into the driver's dispatch table.
 */

void vmwgfx_compat_log(const char *fmt, ...);
void *drm_gem_vmw_mmap(struct drm_device *dev, struct drm_file *f, uint32_t handle);
void drm_gem_vmw_munmap(struct drm_device *dev, struct drm_file *f, uint32_t handle);

extern struct drm_device *current_drm_device;

#define MAX_DRM_FILES 128
static struct drm_file *drm_files[MAX_DRM_FILES] = { NULL };

static int drm_driver_ioctl(int fd, unsigned long index, void *data, unsigned long size)
{
    struct drm_device *dev = current_drm_device;
    const struct drm_driver *drv;
    const struct drm_ioctl_desc *desc;
    int ret;

    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd] || !dev)
        return -EINVAL;

    drv = dev->driver;
    if (!drv || !drv->ioctls)
        return -EINVAL;

    if (index >= drv->num_ioctls)
        return -EINVAL;
    desc = &drv->ioctls[index];
    if (!desc->func)
        return -EINVAL;

    ret = desc->func(dev, data, drm_files[fd]);
    D(if (ret) vmwgfx_compat_log("[vmwgfx] ioctl %lu failed, %d\n", index, ret);)
    return ret;
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

/* dumb buffers: the core's ioctl wrappers are not built, the driver hooks are called directly */
static int drm_dumb_create(struct drm_device *dev, void *arg, struct drm_file *file)
{
    if (!dev->driver->dumb_create)
        return -ENOSYS;
    return dev->driver->dumb_create(file, dev, arg);
}

static int drm_dumb_destroy(struct drm_device *dev, void *arg, struct drm_file *file)
{
    struct drm_mode_destroy_dumb *args = arg;

    return drm_gem_handle_delete(file, args->handle);
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
            case DRM_IOCTL_MODE_CREATE_DUMB: ret = drm_dumb_create(dev, arg, file); break;
            case DRM_IOCTL_MODE_DESTROY_DUMB: ret = drm_dumb_destroy(dev, arg, file); break;
            case DRM_IOCTL_MODE_ADDFB:      ret = drm_mode_addfb_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_ADDFB2:     ret = drm_mode_addfb2_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_RMFB:       ret = drm_mode_rmfb_ioctl(dev, arg, file); break;
            case DRM_IOCTL_MODE_GETFB:      ret = drm_mode_getfb(dev, arg, file); break;
            case DRM_IOCTL_MODE_DIRTYFB:    ret = drm_mode_dirtyfb_ioctl(dev, arg, file); break;
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
            default:
                bug("[vmwgfx] drmIoctl: request 0x%lx not implemented\n", request);
                return -EINVAL;
        }
    } while (ret == -EINTR || ret == -EAGAIN);

    if (ret)
        vmwgfx_compat_log("[vmwgfx] drmIoctl: request 0x%lx failed, %d\n", request, ret);

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

void *drmMMap(int fd, uint32_t handle, VOID (*unmapped)(APTR), APTR data)
{
    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd])
        return NULL;
    return drm_gem_vmw_mmap(current_drm_device, drm_files[fd], handle);
}

void drmMUnmap(int fd, uint32_t handle)
{
    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd])
        return;
    drm_gem_vmw_munmap(current_drm_device, drm_files[fd], handle);
}

BOOL drmGetChipName(int fd, char *name, int namelen)
{
    return FALSE;
}

BOOL drmGetMonitorName(int fd, uint32_t connector_id, char *name, int namelen)
{
    return FALSE;
}
