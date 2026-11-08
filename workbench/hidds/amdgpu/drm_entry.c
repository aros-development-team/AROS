/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The DRM device as a table (include/libdrm/drm_entry.h): what the
    gallium driver library (radeonsi.amdd) reaches the kernel driver
    through, each open() a client of its own, and what AmdgpuDrmEntry()
    exports to programs outside the module.

    Everything here is the in-process libdrm of libdrm/arosdrm.c, wrapped
    so the table carries no libdrm types.
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <errno.h>
#include <string.h>

#include <xf86drm.h>
#include <libdrm/drm_entry.h>

#include "amdgpu_intern.h"

#include LC_LIBDEFS_FILE

/*
 * A new client goes as far as the driver's VM setup: more than an
 * application's task can be assumed to have. A caller with little stack
 * left opens on a stack of its own (the ioctls swap stacks themselves).
 */
#define ENTRY_STACK_NEEDED  (256 * 1024)
#define ENTRY_STACK_SIZE    (1024 * 1024)

static IPTR open_call(IPTR a0, IPTR a1, IPTR a2, IPTR a3)
{
    return (IPTR)drmOpen("amdgpu", "");
}

static int entry_open(void)
{
    struct Task *task = FindTask(NULL);
    struct StackSwapStruct sss;
    struct StackSwapArgs args;
    IPTR here = (IPTR)&sss, ret;

    if (here - (IPTR)task->tc_SPLower >= ENTRY_STACK_NEEDED)
        return (int)open_call(0, 0, 0, 0);

    sss.stk_Lower = AllocMem(ENTRY_STACK_SIZE, MEMF_ANY);
    if (!sss.stk_Lower)
        return -ENOMEM;
    sss.stk_Upper = (APTR)((IPTR)sss.stk_Lower + ENTRY_STACK_SIZE);
    sss.stk_Pointer = sss.stk_Upper;
    memset(&args, 0, sizeof(args));
    ret = NewStackSwap(&sss, open_call, &args);
    FreeMem(sss.stk_Lower, ENTRY_STACK_SIZE);
    return (int)ret;
}

static int entry_get_version(int fd, int *major, int *minor, int *patchlevel,
                             char *name, int namelen)
{
    drmVersionPtr ver = drmGetVersion(fd);

    if (!ver || !ver->name)
        return -ENODEV;
    *major = ver->version_major;
    *minor = ver->version_minor;
    *patchlevel = ver->version_patchlevel;
    if (name && namelen > 0)
    {
        strncpy(name, ver->name, namelen - 1);
        name[namelen - 1] = 0;
    }
    drmFreeVersion(ver);
    return 0;
}

static int entry_get_pci_info(int fd, struct amdgpu_drm_pci_info *info)
{
    drmDevicePtr dev = NULL;
    int ret = drmGetDevice2(fd, 0, &dev);

    if (ret)
        return ret;
    info->domain       = dev->businfo.pci->domain;
    info->bus          = dev->businfo.pci->bus;
    info->dev          = dev->businfo.pci->dev;
    info->func         = dev->businfo.pci->func;
    info->vendor_id    = dev->deviceinfo.pci->vendor_id;
    info->device_id    = dev->deviceinfo.pci->device_id;
    info->subvendor_id = dev->deviceinfo.pci->subvendor_id;
    info->subdevice_id = dev->deviceinfo.pci->subdevice_id;
    info->revision_id  = dev->deviceinfo.pci->revision_id;
    drmFreeDevice(&dev);
    return 0;
}

static void *entry_map(int fd, uint32_t handle)
{
    return drmMMap(fd, handle, NULL, NULL);
}

static const struct amdgpu_drm_entry drm_entry =
{
    .version      = AMDGPU_DRM_ENTRY_VERSION,
    .size         = sizeof(struct amdgpu_drm_entry),
    .open         = entry_open,
    .close        = drmClose,
    .ioctl        = drmIoctl,
    .command      = drmCommandWriteRead,
    .get_version  = entry_get_version,
    .get_pci_info = entry_get_pci_info,
    .map          = entry_map,
    .unmap        = drmMUnmap,
    .dup          = drmDupFd,
};

const struct amdgpu_drm_entry *HIDDAmdgpuDrmEntry(void)
{
    return &drm_entry;
}

AROS_LH0(APTR, AmdgpuDrmEntry,
         LIBBASETYPEPTR, LIBBASE, 5, Amdgpu)
{
    AROS_LIBFUNC_INIT

    return (APTR)&drm_entry;

    AROS_LIBFUNC_EXIT
}
