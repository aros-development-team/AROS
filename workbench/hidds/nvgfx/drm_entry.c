/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The DRM device as a table (include/libdrm/drm_entry.h): what the
    Vulkan driver library (NVK, nvk.nvdd: nvk/nvk_nvdd.c)
    reaches the kernel driver through, each open() a client of its own,
    and what NouveauDrmEntry() exports to test programs outside it.
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

#include <libdrm/arosdrm.h>
#include <xf86drm.h>
#include <libdrm/drm_entry.h>

#include "nvgfx_intern.h"

/*
 * The driver runs on the caller's stack, and a channel allocation or a
 * VM_INIT goes as far as a GSP RPC: more than an application's task can
 * be assumed to have. A caller with little stack left runs the call on
 * a stack of its own.
 */
#define ENTRY_STACK_NEEDED  (256 * 1024)
#define ENTRY_STACK_SIZE    (1024 * 1024)

static IPTR entry_call(APTR func, IPTR a0, IPTR a1, IPTR a2, IPTR a3)
{
    struct Task *task = FindTask(NULL);
    struct StackSwapStruct sss;
    struct StackSwapArgs args;
    IPTR here = (IPTR)&sss, ret;

    if (here - (IPTR)task->tc_SPLower >= ENTRY_STACK_NEEDED)
        return ((IPTR (*)(IPTR, IPTR, IPTR, IPTR))func)(a0, a1, a2, a3);

    sss.stk_Lower = AllocMem(ENTRY_STACK_SIZE, MEMF_ANY);
    if (!sss.stk_Lower)
        return (IPTR)-ENOMEM;
    sss.stk_Upper = (APTR)((IPTR)sss.stk_Lower + ENTRY_STACK_SIZE);
    sss.stk_Pointer = sss.stk_Upper;
    args.Args[0] = a0;
    args.Args[1] = a1;
    args.Args[2] = a2;
    args.Args[3] = a3;
    ret = NewStackSwap(&sss, func, &args);
    FreeMem(sss.stk_Lower, ENTRY_STACK_SIZE);
    return ret;
}

static IPTR open_call(IPTR a0, IPTR a1, IPTR a2, IPTR a3)
{
    return (IPTR)drmOpen("nouveau", "");
}

static int entry_open(void)
{
    return (int)entry_call(open_call, 0, 0, 0, 0);
}

static IPTR ioctl_call(IPTR fd, IPTR request, IPTR arg, IPTR a3)
{
    return (IPTR)drmIoctl((int)fd, (unsigned long)request, (void *)arg);
}

static int entry_ioctl(int fd, unsigned long request, void *arg)
{
    return (int)entry_call(ioctl_call, (IPTR)fd, (IPTR)request, (IPTR)arg, 0);
}

static IPTR command_call(IPTR fd, IPTR index, IPTR data, IPTR size)
{
    return (IPTR)drmCommandWriteRead((int)fd, (unsigned long)index, (void *)data, (unsigned long)size);
}

static int entry_command(int fd, unsigned long index, void *data, unsigned long size)
{
    return (int)entry_call(command_call, (IPTR)fd, (IPTR)index, (IPTR)data, (IPTR)size);
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

static int entry_get_pci_info(int fd, struct nouveau_drm_pci_info *info)
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

static void *entry_map_notify(int fd, uint32_t handle, void (*unmapped)(void *), void *data)
{
    return drmMMap(fd, handle, (VOID (*)(APTR))unmapped, data);
}

/* The display's DRM client: scan-out bitmaps' buffers are its GEM objects */
static int entry_display_fd = -1;

void HIDDNouveauDrmEntrySetDisplay(int display_fd)
{
    entry_display_fd = display_fd;
}

/* A display buffer in another client: its global (flink) name from the
 * display's client, opened in fd's. Both clients belong to the one DRM
 * device, so this shares the buffer itself - nothing is copied. */
static IPTR import_call(IPTR fd, IPTR display_handle, IPTR handle_out, IPTR a3)
{
    struct drm_gem_flink flink = { .handle = (uint32_t)display_handle };
    struct drm_gem_open open = { 0 };

    if (entry_display_fd < 0)
        return (IPTR)-ENODEV;
    if (drmIoctl(entry_display_fd, DRM_IOCTL_GEM_FLINK, &flink) || !flink.name)
        return (IPTR)-ENOENT;
    open.name = flink.name;
    if (drmIoctl((int)fd, DRM_IOCTL_GEM_OPEN, &open) || !open.handle)
        return (IPTR)-ENOENT;
    *(uint32_t *)handle_out = open.handle;
    return 0;
}

static int entry_import_display_handle(int fd, uint32_t display_handle, uint32_t *handle)
{
    return (int)(SIPTR)entry_call(import_call, (IPTR)fd, (IPTR)display_handle, (IPTR)handle, 0);
}

static const struct nouveau_drm_entry drm_entry =
{
    .version      = NOUVEAU_DRM_ENTRY_VERSION,
    .size         = sizeof(struct nouveau_drm_entry),
    .open         = entry_open,
    .close        = drmClose,
    .ioctl        = entry_ioctl,
    .command      = entry_command,
    .get_version  = entry_get_version,
    .get_pci_info = entry_get_pci_info,
    .map          = entry_map,
    .unmap        = drmMUnmap,
    .import_display_handle = entry_import_display_handle,
    .map_notify   = entry_map_notify,
};

const struct nouveau_drm_entry *HIDDNouveauDrmEntry(void)
{
    return &drm_entry;
}

AROS_LH0(APTR, NouveauDrmEntry,
         LIBBASETYPEPTR, LIBBASE, 5, Nouveau)
{
    AROS_LIBFUNC_INIT

    return (APTR)&drm_entry;

    AROS_LIBFUNC_EXIT
}
