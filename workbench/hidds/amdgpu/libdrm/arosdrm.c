/*
    Copyright 2010-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <exec/lists.h>
#include <exec/tasks.h>

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

void amdgpu_compat_log(const char *fmt, ...);
void *drm_gem_amdgpu_mmap(struct drm_device *dev, struct drm_file *f, uint32_t handle);
void drm_gem_amdgpu_munmap(struct drm_device *dev, struct drm_file *f, uint32_t handle);

extern struct drm_device *current_drm_device;

#define MAX_DRM_FILES 128
static struct drm_file *drm_files[MAX_DRM_FILES] = { NULL };

#define DRM_STACK_SIZE  (256 * 1024)
#define DRM_STACK_MIN   (128 * 1024)

static struct SignalSemaphore drm_stack_lock;
static struct MinList drm_stack_pool;
static volatile BOOL drm_stack_ready;

static APTR drm_stack_get(void)
{
    struct MinNode *n;

    if (!drm_stack_ready)
    {
        Forbid();
        if (!drm_stack_ready)
        {
            InitSemaphore(&drm_stack_lock);
            NEWLIST((struct List *)&drm_stack_pool);
            drm_stack_ready = TRUE;
        }
        Permit();
    }
    ObtainSemaphore(&drm_stack_lock);
    n = (struct MinNode *)REMHEAD((struct List *)&drm_stack_pool);
    ReleaseSemaphore(&drm_stack_lock);
    return n ? (APTR)n : AllocMem(DRM_STACK_SIZE, MEMF_ANY);
}

static void drm_stack_put(APTR stack)
{
    ObtainSemaphore(&drm_stack_lock);
    ADDHEAD((struct List *)&drm_stack_pool, (struct Node *)stack);
    ReleaseSemaphore(&drm_stack_lock);
}

/* the Linux driver expects a kernel-sized stack; callers such as
   input.device have a few tens of KB */
static IPTR drm_call(APTR func, IPTR a, IPTR b, IPTR c, IPTR d)
{
    struct Task *me = FindTask(NULL);
    UBYTE *sp = (UBYTE *)__builtin_frame_address(0);
    struct StackSwapStruct sss;
    struct StackSwapArgs args;
    APTR stack;
    IPTR ret;

    if (sp - (UBYTE *)me->tc_SPLower >= DRM_STACK_MIN || !(stack = drm_stack_get()))
        return ((IPTR (*)(IPTR, IPTR, IPTR, IPTR))func)(a, b, c, d);

    sss.stk_Lower = stack;
    sss.stk_Upper = (UBYTE *)stack + DRM_STACK_SIZE;
    sss.stk_Pointer = sss.stk_Upper;
    memset(&args, 0, sizeof(args));
    args.Args[0] = a;
    args.Args[1] = b;
    args.Args[2] = c;
    args.Args[3] = d;
    ret = NewStackSwap(&sss, func, &args);
    drm_stack_put(stack);
    return ret;
}

static int drm_driver_ioctl_impl(int fd, unsigned long index, void *data, unsigned long size)
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
    D(if (ret) amdgpu_compat_log("[amdgpu] ioctl %lu failed, %d\n", index, ret);)
    return ret;
}

static int drm_driver_ioctl(int fd, unsigned long index, void *data, unsigned long size)
{
    return (int)drm_call(drm_driver_ioctl_impl, fd, index, (IPTR)data, size);
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

/* capabilities answered here: the core's GET_CAP handler is not built */
static int drm_getcap(struct drm_device *dev, struct drm_get_cap *req)
{
    switch (req->capability)
    {
        case DRM_CAP_DUMB_BUFFER:           req->value = 1; break;
        case DRM_CAP_DUMB_PREFERRED_DEPTH:  req->value = 24; break;
        case DRM_CAP_DUMB_PREFER_SHADOW:    req->value = 0; break;
        case DRM_CAP_PRIME:                 req->value = 0; break;
        case DRM_CAP_TIMESTAMP_MONOTONIC:   req->value = 1; break;
        case DRM_CAP_ADDFB2_MODIFIERS:      req->value = 1; break;
        case DRM_CAP_SYNCOBJ:               req->value = 1; break;
        /* fence chains are not carried, so no timeline syncobjs */
        case DRM_CAP_SYNCOBJ_TIMELINE:      req->value = 0; break;
        case DRM_CAP_CURSOR_WIDTH:
        case DRM_CAP_CURSOR_HEIGHT:         req->value = 64; break;
        default:                            req->value = 0; return -EINVAL;
    }
    return 0;
}

static int drmIoctl_impl(int fd, unsigned long request, void *arg)
{
    struct drm_device *dev = current_drm_device;
    struct drm_file *file;
    int ret = -EINVAL;

    if (fd < 0 || fd >= MAX_DRM_FILES || !(file = drm_files[fd]))
        return ret;

    if (DRM_IOCTL_NR(request) >= DRM_COMMAND_BASE && DRM_IOCTL_NR(request) < DRM_COMMAND_END)
        return drm_driver_ioctl(fd, DRM_IOCTL_NR(request) - DRM_COMMAND_BASE, arg, _IOC_SIZE(request));

    do
    {
        /* by command number, as Linux does: AROS has two ioctl encodings */
        switch (DRM_IOCTL_NR(request))
        {
            case DRM_IOCTL_NR(DRM_IOCTL_GET_CLIENT):
            {
                struct drm_client *client = arg;

                /* only the caller's own file, which is always authenticated */
                if (client->idx != 0)
                    return -EINVAL;
                client->auth = 1;
                client->pid = 0;
                client->uid = 0;
                client->magic = 0;
                client->iocs = 0;
                ret = 0;
                break;
            }
            case DRM_IOCTL_NR(DRM_IOCTL_GET_CAP):         ret = drm_getcap(dev, arg); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_CREATE):  ret = drm_syncobj_create_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_DESTROY): ret = drm_syncobj_destroy_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_WAIT):    ret = drm_syncobj_wait_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_TIMELINE_WAIT): ret = drm_syncobj_timeline_wait_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_RESET):   ret = drm_syncobj_reset_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_SIGNAL):  ret = drm_syncobj_signal_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_TIMELINE_SIGNAL): ret = drm_syncobj_timeline_signal_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_QUERY):   ret = drm_syncobj_query_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_TRANSFER): ret = drm_syncobj_transfer_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD):
            case DRM_IOCTL_NR(DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE):
            case DRM_IOCTL_NR(DRM_IOCTL_PRIME_HANDLE_TO_FD):
            case DRM_IOCTL_NR(DRM_IOCTL_PRIME_FD_TO_HANDLE):
                return -ENOSYS;
            case DRM_IOCTL_NR(DRM_IOCTL_GEM_CLOSE):       ret = drm_gem_close_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_GEM_OPEN):        ret = drm_gem_open_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_GEM_FLINK):       ret = drm_gem_flink_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_CREATE_DUMB): ret = drm_dumb_create(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_DESTROY_DUMB): ret = drm_dumb_destroy(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_ADDFB):      ret = drm_mode_addfb_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_ADDFB2):     ret = drm_mode_addfb2_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_RMFB):       ret = drm_mode_rmfb_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETFB):      ret = drm_mode_getfb(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_DIRTYFB):    ret = drm_mode_dirtyfb_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_SETCRTC):    ret = drm_mode_setcrtc(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETCRTC):    ret = drm_mode_getcrtc(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETRESOURCES): ret = drm_mode_getresources(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETCONNECTOR): ret = drm_mode_getconnector(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_CURSOR):     ret = drm_mode_cursor_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_CURSOR2):    ret = drm_mode_cursor2_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETENCODER): ret = drm_mode_getencoder(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETPLANERESOURCES): ret = drm_mode_getplane_res(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETPLANE):   ret = drm_mode_getplane(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_SETPLANE):   ret = drm_mode_setplane(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETPROPERTY): ret = drm_mode_getproperty_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETPROPBLOB): ret = drm_mode_getblob_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_OBJ_GETPROPERTIES): ret = drm_mode_obj_get_properties_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_OBJ_SETPROPERTY): ret = drm_mode_obj_set_property_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_ATOMIC):     ret = drm_mode_atomic_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_GETGAMMA):   ret = drm_mode_gamma_get_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_SETGAMMA):   ret = drm_mode_gamma_set_ioctl(dev, arg, file); break;
            case DRM_IOCTL_NR(DRM_IOCTL_MODE_PAGE_FLIP):  ret = drm_mode_page_flip_ioctl(dev, arg, file); break;
            default:
                bug("[amdgpu] drmIoctl: request 0x%lx not implemented\n", request);
                return -EINVAL;
        }
    } while (ret == -EINTR || ret == -EAGAIN);

    if (ret)
        amdgpu_compat_log("[amdgpu] drmIoctl: request 0x%lx failed, %d\n", request, ret);

    return ret;
}

int drmIoctl(int fd, unsigned long request, void *arg)
{
    return (int)drm_call(drmIoctl_impl, fd, request, (IPTR)arg, 0);
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
    return (void *)drm_call(drm_gem_amdgpu_mmap, (IPTR)current_drm_device, (IPTR)drm_files[fd], handle, 0);
}

void drmMUnmap(int fd, uint32_t handle)
{
    if (fd < 0 || fd >= MAX_DRM_FILES || !drm_files[fd])
        return;
    drm_call(drm_gem_amdgpu_munmap, (IPTR)current_drm_device, (IPTR)drm_files[fd], handle, 0);
}

BOOL drmGetChipName(int fd, char *name, int namelen)
{
    return FALSE;
}

BOOL drmGetMonitorName(int fd, uint32_t connector_id, char *name, int namelen)
{
    return FALSE;
}

/* --- the rest of libdrm's interface the amdgpu winsys uses ---------------- */

int drmGetCap(int fd, uint64_t capability, uint64_t *value)
{
    struct drm_get_cap req = { .capability = capability };
    int ret = drmIoctl(fd, DRM_IOCTL_GET_CAP, &req);

    if (ret == 0)
        *value = req.value;
    return ret;
}

int drmGetDevice2(int fd, uint32_t flags, drmDevicePtr *device)
{
    struct pci_dev *pdev;
    drmDevicePtr d;

    if (!current_drm_device || !current_drm_device->dev || !dev_is_pci(current_drm_device->dev))
        return -ENODEV;
    pdev = to_pci_dev(current_drm_device->dev);

    d = kzalloc(sizeof(*d) + sizeof(drmPciBusInfo) + sizeof(drmPciDeviceInfo) + 2 * sizeof(char *) + 32, GFP_KERNEL);
    if (!d)
        return -ENOMEM;
    d->businfo.pci = (drmPciBusInfoPtr)(d + 1);
    d->deviceinfo.pci = (drmPciDeviceInfoPtr)(d->businfo.pci + 1);
    d->nodes = (char **)(d->deviceinfo.pci + 1);
    d->nodes[DRM_NODE_PRIMARY] = (char *)(d->nodes + 2);
    strcpy(d->nodes[DRM_NODE_PRIMARY], "/dev/dri/card0");
    d->available_nodes = 1 << DRM_NODE_PRIMARY;
    d->bustype = DRM_BUS_PCI;
    d->businfo.pci->domain = 0;
    d->businfo.pci->bus = pdev->bus->number;
    d->businfo.pci->dev = PCI_SLOT(pdev->devfn);
    d->businfo.pci->func = PCI_FUNC(pdev->devfn);
    d->deviceinfo.pci->vendor_id = pdev->vendor;
    d->deviceinfo.pci->device_id = pdev->device;
    d->deviceinfo.pci->subvendor_id = pdev->subsystem_vendor;
    d->deviceinfo.pci->subdevice_id = pdev->subsystem_device;
    d->deviceinfo.pci->revision_id = pdev->revision;
    *device = d;
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

int drmGetNodeTypeFromFd(int fd)
{
    return DRM_NODE_PRIMARY;
}

char *drmGetPrimaryDeviceNameFromFd(int fd)
{
    char *name = kmalloc(16, GFP_KERNEL);

    if (name)
        strcpy(name, "/dev/dri/card0");
    return name;
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
    amdgpu_compat_log("[libdrm] %s", buf);
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
