/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    libdrm for NVK on AROS. NVK is the nvgfx driver's library nvk.nvdd
    (hidds/nvgfx/nvk); it reaches the device through
    nouveau's DRM entry table (hidds/nvgfx/include/libdrm/drm_entry.h:
    a client of its own per open, calls on a stack deep enough for the
    driver), handed in with nvk_drm_set_entry() before the first Vulkan
    call. The functions here carry libdrm's names in NVK's sources and
    nvk_ ones in the object (nvk_drm_names.h). Everything else is libdrm's
    own logic over that table: the syncobj wrappers are the ioctl
    sequences of xf86drm.c, the device enumeration describes the one card
    the driver probed.
*/

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <xf86drm.h>

#include "libdrm/drm_entry.h"

#include "nvk_drm.h"

static const struct nouveau_drm_entry *drm_entry;

void
nvk_drm_set_entry(const void *entry)
{
   drm_entry = entry;
}

bool
nvk_drm_available(void)
{
   return drm_entry != NULL && drm_entry->version == NOUVEAU_DRM_ENTRY_VERSION;
}

/* ---- device access ---- */

int
drmOpen(const char *name, const char *busid)
{
   (void)name;
   (void)busid;
   if (!nvk_drm_available())
      return -ENODEV;
   return drm_entry->open();
}

int
drmClose(int fd)
{
   if (!nvk_drm_available())
      return -ENODEV;
   return drm_entry->close(fd);
}

int
drmIoctl(int fd, unsigned long request, void *arg)
{
   int ret;

   /* libdrm's convention, which the syncobj code tests errno against
    * (ETIME is a timeout): -1 with errno set. The table returns -errno. */
   ret = nvk_drm_available() ? drm_entry->ioctl(fd, request, arg) : -ENODEV;
   if (ret < 0) {
      errno = -ret;
      return -1;
   }
   return ret;
}

static int
drm_command(int fd, unsigned long index, void *data, unsigned long size)
{
   int ret;

   /* libdrm's drmCommand*() return -errno and leave errno set too */
   ret = nvk_drm_available() ? drm_entry->command(fd, index, data, size) : -ENODEV;
   if (ret < 0)
      errno = -ret;
   return ret;
}

int
drmCommandNone(int fd, unsigned long drmCommandIndex)
{
   return drm_command(fd, drmCommandIndex, NULL, 0);
}

int
drmCommandRead(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
   return drm_command(fd, drmCommandIndex, data, size);
}

int
drmCommandWrite(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
   return drm_command(fd, drmCommandIndex, data, size);
}

int
drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
   return drm_command(fd, drmCommandIndex, data, size);
}

drmVersionPtr
drmGetVersion(int fd)
{
   static char name[32];
   static drmVersion ver;

   if (!nvk_drm_available())
      return NULL;

   memset(&ver, 0, sizeof(ver));
   if (drm_entry->get_version(fd, &ver.version_major, &ver.version_minor,
                              &ver.version_patchlevel, name, sizeof(name)))
      return NULL;

   ver.name = name;
   ver.name_len = strlen(name);
   return &ver;
}

void
drmFreeVersion(drmVersionPtr ver)
{
   (void)ver;
}

int
drmGetCap(int fd, uint64_t capability, uint64_t *value)
{
   struct drm_get_cap cap = { .capability = capability };
   int ret = drmIoctl(fd, DRM_IOCTL_GET_CAP, &cap);

   if (ret == 0) {
      *value = cap.value;
      return 0;
   }

   /* The kernel side answers through its own driver table; the syncobj
    * support it carries is what these two capabilities report. */
   switch (capability) {
   case DRM_CAP_SYNCOBJ:
   case DRM_CAP_SYNCOBJ_TIMELINE:
      *value = 1;
      return 0;
   default:
      return ret;
   }
}

int
drmGetMagic(int fd, drm_magic_t *magic)
{
   (void)fd;
   *magic = 0;
   return -EINVAL;
}

/* ---- device enumeration: the one card the nvgfx driver drives ---- */

static const char *node_names[DRM_NODE_MAX] = { "nouveau", NULL, "nouveau" };

static int
device_describe(int fd, drmDevicePtr *out)
{
   struct nouveau_drm_pci_info info;
   struct {
      drmDevice dev;
      drmPciBusInfo bus;
      drmPciDeviceInfo pci;
      char *nodes[DRM_NODE_MAX];
   } *d;

   if (!nvk_drm_available())
      return -ENODEV;

   if (drm_entry->get_pci_info(fd, &info))
      return -ENODEV;

   d = calloc(1, sizeof(*d));
   if (!d)
      return -ENOMEM;

   d->nodes[DRM_NODE_PRIMARY] = (char *)node_names[DRM_NODE_PRIMARY];
   d->nodes[DRM_NODE_RENDER] = (char *)node_names[DRM_NODE_RENDER];
   d->dev.nodes = d->nodes;
   d->dev.available_nodes = (1 << DRM_NODE_PRIMARY) | (1 << DRM_NODE_RENDER);
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

   *out = &d->dev;
   return 0;
}

int
drmGetDevice2(int fd, uint32_t flags, drmDevicePtr *device)
{
   (void)flags;
   return device_describe(fd, device);
}

int
drmGetDevices2(uint32_t flags, drmDevicePtr devices[], int max_devices)
{
   int fd, ret;

   (void)flags;
   if (max_devices < 1)
      return 0;

   fd = drmOpen("nouveau", "");
   if (fd < 0)
      return 0;

   ret = device_describe(fd, &devices[0]);
   drmClose(fd);

   return ret ? 0 : 1;
}

int
drmGetDeviceFromDevId(dev_t find_rdev, uint32_t flags, drmDevicePtr *device)
{
   int fd, ret;

   (void)find_rdev;
   (void)flags;

   fd = drmOpen("nouveau", "");
   if (fd < 0)
      return -ENODEV;

   ret = device_describe(fd, device);
   drmClose(fd);
   return ret;
}

void
drmFreeDevice(drmDevicePtr *device)
{
   if (device && *device) {
      free(*device);
      *device = NULL;
   }
}

void
drmFreeDevices(drmDevicePtr devices[], int count)
{
   for (int i = 0; i < count; i++)
      drmFreeDevice(&devices[i]);
}

int
drmGetNodeTypeFromFd(int fd)
{
   (void)fd;
   return DRM_NODE_RENDER;
}

char *
drmGetDeviceNameFromFd2(int fd)
{
   (void)fd;
   return strdup(node_names[DRM_NODE_RENDER]);
}

char *
drmGetRenderDeviceNameFromFd(int fd)
{
   (void)fd;
   return strdup(node_names[DRM_NODE_RENDER]);
}

/* ---- GEM ---- */

int
drmCloseBufferHandle(int fd, uint32_t handle)
{
   struct drm_gem_close args = { .handle = handle };

   return drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &args);
}

int
drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd)
{
   (void)fd; (void)handle; (void)flags; (void)prime_fd;
   return -ENOSYS;
}

/*
 * There are no dma-buf fds on AROS. What travels in their place is a
 * display buffer's GEM handle (a scan-out bitmap's, from the display's own
 * DRM client): the driver's entry opens that buffer in this client.
 */
int
drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle)
{
   int ret;

   if (!nvk_drm_available() ||
       drm_entry->size < offsetof(struct nouveau_drm_entry, import_display_handle) +
                         sizeof(drm_entry->import_display_handle) ||
       !drm_entry->import_display_handle) {
      errno = ENOSYS;
      return -1;
   }
   ret = drm_entry->import_display_handle(fd, (uint32_t)prime_fd, handle);
   if (ret < 0) {
      errno = -ret;
      return -1;
   }
   return 0;
}

void *
drmMMap(int fd, uint32_t handle)
{
   if (!nvk_drm_available())
      return NULL;
   return drm_entry->map(fd, handle);
}

void
drmMUnmap(int fd, uint32_t handle)
{
   if (!nvk_drm_available())
      return;
   drm_entry->unmap(fd, handle);
}

/* ---- syncobj, as xf86drm.c does it ---- */

int
drmSyncobjCreate(int fd, uint32_t flags, uint32_t *handle)
{
   struct drm_syncobj_create args = { .flags = flags };
   int ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_CREATE, &args);

   if (ret)
      return ret;
   *handle = args.handle;
   return 0;
}

int
drmSyncobjDestroy(int fd, uint32_t handle)
{
   struct drm_syncobj_destroy args = { .handle = handle };

   return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_DESTROY, &args);
}

int
drmSyncobjHandleToFD(int fd, uint32_t handle, int *obj_fd)
{
   struct drm_syncobj_handle args = { .handle = handle };
   int ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, &args);

   if (ret)
      return ret;
   *obj_fd = args.fd;
   return 0;
}

int
drmSyncobjFDToHandle(int fd, int obj_fd, uint32_t *handle)
{
   struct drm_syncobj_handle args = { .fd = obj_fd };
   int ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, &args);

   if (ret)
      return ret;
   *handle = args.handle;
   return 0;
}

int
drmSyncobjImportSyncFile(int fd, uint32_t handle, int sync_file_fd)
{
   struct drm_syncobj_handle args = {
      .handle = handle,
      .flags = DRM_SYNCOBJ_FD_TO_HANDLE_FLAGS_IMPORT_SYNC_FILE,
      .fd = sync_file_fd,
   };

   return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, &args);
}

int
drmSyncobjExportSyncFile(int fd, uint32_t handle, int *sync_file_fd)
{
   struct drm_syncobj_handle args = {
      .handle = handle,
      .flags = DRM_SYNCOBJ_HANDLE_TO_FD_FLAGS_EXPORT_SYNC_FILE,
   };
   int ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, &args);

   if (ret)
      return ret;
   *sync_file_fd = args.fd;
   return 0;
}

int
drmSyncobjWait(int fd, uint32_t *handles, unsigned num_handles,
               int64_t timeout_nsec, unsigned flags, uint32_t *first_signaled)
{
   struct drm_syncobj_wait args = {
      .handles = (uintptr_t)handles,
      .timeout_nsec = timeout_nsec,
      .count_handles = num_handles,
      .flags = flags,
   };
   int ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_WAIT, &args);

   if (ret)
      return ret;
   if (first_signaled)
      *first_signaled = args.first_signaled;
   return 0;
}

int
drmSyncobjReset(int fd, const uint32_t *handles, uint32_t handle_count)
{
   struct drm_syncobj_array args = {
      .handles = (uintptr_t)handles,
      .count_handles = handle_count,
   };

   return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_RESET, &args);
}

int
drmSyncobjSignal(int fd, const uint32_t *handles, uint32_t handle_count)
{
   struct drm_syncobj_array args = {
      .handles = (uintptr_t)handles,
      .count_handles = handle_count,
   };

   return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_SIGNAL, &args);
}

int
drmSyncobjTimelineSignal(int fd, const uint32_t *handles, uint64_t *points,
                         uint32_t handle_count)
{
   struct drm_syncobj_timeline_array args = {
      .handles = (uintptr_t)handles,
      .points = (uintptr_t)points,
      .count_handles = handle_count,
   };

   return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_TIMELINE_SIGNAL, &args);
}

int
drmSyncobjTimelineWait(int fd, uint32_t *handles, uint64_t *points,
                       unsigned num_handles, int64_t timeout_nsec, unsigned flags,
                       uint32_t *first_signaled)
{
   struct drm_syncobj_timeline_wait args = {
      .handles = (uintptr_t)handles,
      .points = (uintptr_t)points,
      .timeout_nsec = timeout_nsec,
      .count_handles = num_handles,
      .flags = flags,
   };
   int ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_TIMELINE_WAIT, &args);

   if (ret)
      return ret;
   if (first_signaled)
      *first_signaled = args.first_signaled;
   return 0;
}

int
drmSyncobjQuery(int fd, uint32_t *handles, uint64_t *points, uint32_t handle_count)
{
   return drmSyncobjQuery2(fd, handles, points, handle_count, 0);
}

int
drmSyncobjQuery2(int fd, uint32_t *handles, uint64_t *points, uint32_t handle_count,
                 uint32_t flags)
{
   struct drm_syncobj_timeline_array args = {
      .handles = (uintptr_t)handles,
      .points = (uintptr_t)points,
      .count_handles = handle_count,
      .flags = flags,
   };

   return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_QUERY, &args);
}

int
drmSyncobjTransfer(int fd, uint32_t dst_handle, uint64_t dst_point,
                   uint32_t src_handle, uint64_t src_point, uint32_t flags)
{
   struct drm_syncobj_transfer args = {
      .src_handle = src_handle,
      .dst_handle = dst_handle,
      .src_point = src_point,
      .dst_point = dst_point,
      .flags = flags,
   };

   return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_TRANSFER, &args);
}

int
drmSyncobjEventfd(int fd, uint32_t handle, uint64_t point, int ev_fd, uint32_t flags)
{
   struct drm_syncobj_eventfd args = {
      .handle = handle,
      .flags = flags,
      .point = point,
      .fd = ev_fd,
   };

   return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_EVENTFD, &args);
}
