/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosdrm.h"
#include "drmP.h"

extern struct drm_driver * current_drm_driver;

/* FIXME: Array for now, list maybe in future */
struct drm_file * drm_files[128] = {NULL};

int 
drmCommandNone(int fd, unsigned long drmCommandIndex)
{
    if (!drm_files[fd])
        return -EINVAL;

    if (!current_drm_driver || !current_drm_driver->ioctls)
        return -EINVAL;

    return current_drm_driver->ioctls[drmCommandIndex].func(current_drm_driver->dev, NULL, drm_files[fd]);
}

int
drmCommandRead(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
    if (!drm_files[fd])
        return -EINVAL;

    if (!current_drm_driver || !current_drm_driver->ioctls)
        return -EINVAL;

    return current_drm_driver->ioctls[drmCommandIndex].func(current_drm_driver->dev, data, drm_files[fd]);
}

int
drmCommandWrite(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
    if (!drm_files[fd])
        return -EINVAL;

    if (!current_drm_driver || !current_drm_driver->ioctls)
        return -EINVAL;
    
    return current_drm_driver->ioctls[drmCommandIndex].func(current_drm_driver->dev, data, drm_files[fd]);
}

int
drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
    if (!drm_files[fd])
        return -EINVAL;
    
    if (!current_drm_driver || !current_drm_driver->ioctls)
        return -EINVAL;
    
    return current_drm_driver->ioctls[drmCommandIndex].func(current_drm_driver->dev, data, drm_files[fd]);
}

int
drmOpen(const char *name, const char *busid)
{
    int i;

    for (i = 0; i < 128; i++)
    {
        if (drm_files[i] == NULL)
        {
            drm_files[i] = HIDDNouveauAlloc(sizeof(struct drm_file));
            spin_lock_init(&drm_files[i]->table_lock);
            INIT_LIST_HEAD(&drm_files[i]->fbs);
            if (current_drm_driver->open)
                current_drm_driver->open(current_drm_driver->dev, drm_files[i]);
            return i;
        }
    }
    
    return -EINVAL;
}

int
drmClose(int fd)
{
    struct drm_file * f = NULL;

    if (!(f = drm_files[fd]))
        return 0;
    
    drm_files[fd] = NULL;
    
    if (current_drm_driver->preclose)
        current_drm_driver->preclose(current_drm_driver->dev, f);

    if (current_drm_driver->postclose)
        current_drm_driver->postclose(current_drm_driver->dev, f);

    HIDDNouveauFree(f);
    
    return 0;
}

drmVersionPtr
drmGetVersion(int fd)
{
    static drmVersion ver;
    if (current_drm_driver)
        ver.version_patchlevel = current_drm_driver->version_patchlevel;
    else
        ver.version_patchlevel = 0;
    
    return &ver;
}

void
drmFreeVersion(drmVersionPtr ptr)
{
    /* This is a no-op for now */
}

int
drmCreateContext(int fd, drm_context_t * handle)
{
    /* No Op */
    return 0;
}

int
drmDestroyContext(int fd, drm_context_t handle)
{
    /* No Op */
    return 0;
}

int drmIoctl(int fd, unsigned long request, void *arg)
{
    int ret = -EINVAL;
    
    if (!drm_files[fd])
        return ret;

    do
    {
        switch(request)
        {
            case(DRM_IOCTL_GEM_CLOSE):
                ret = drm_gem_close_ioctl(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_GEM_OPEN):
                ret = drm_gem_open_ioctl(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_GEM_FLINK):
                ret = drm_gem_flink_ioctl(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_MODE_ADDFB):
                ret = drm_mode_addfb(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_MODE_RMFB):
                ret = drm_mode_rmfb(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_MODE_SETCRTC):
                ret = drm_mode_setcrtc(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_MODE_GETCRTC):
                ret = drm_mode_getcrtc(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_MODE_GETRESOURCES):
                ret = drm_mode_getresources(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_MODE_GETCONNECTOR):
                ret = drm_mode_getconnector(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_MODE_CURSOR):
                ret = drm_mode_cursor_ioctl(current_drm_driver->dev, arg, drm_files[fd]);
                break;
            case(DRM_IOCTL_MODE_GETENCODER):
                ret = drm_mode_getencoder(current_drm_driver->dev, arg, drm_files[fd]);
                break;            
            default:
                DRM_IMPL("GEM COMMAND %d\n", request);
        }
        /* FIXME: It is possible that -ERESTARTSYS needs to be translated to -EAGAIN here */
    } while (ret == -EINTR || ret == -EAGAIN);
    
    return ret;
}

void *drmMalloc(int size)
{
    return HIDDNouveauAlloc(size);
}

void drmFree(void *pt)
{
    HIDDNouveauFree(pt);
}
