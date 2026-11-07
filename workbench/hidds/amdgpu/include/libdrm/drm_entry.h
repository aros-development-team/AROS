/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The DRM device as a table.

    amdgpu.hidd carries the kernel driver and its libdrm; the display code
    reaches them by static linkage. Code with a libdrm of its own goes
    through this table instead: the gallium driver library (radeonsi.amdd,
    over the host it is given), and programs outside the module, through
    the one function amdgpu.hidd exports, AmdgpuDrmEntry().

    Every entry keeps the semantics of the libdrm function named beside
    it. An fd is what open() hands out and has meaning only through this
    table; it is not a file descriptor of any C library.
*/

#ifndef LIBDRM_DRM_ENTRY_H
#define LIBDRM_DRM_ENTRY_H

#include <exec/types.h>
#include <aros/libcall.h>
#include <stdint.h>

#define AMDGPU_DRM_ENTRY_VERSION   1

struct amdgpu_drm_pci_info
{
    uint16_t domain;
    uint8_t  bus;
    uint8_t  dev;
    uint8_t  func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t subvendor_id;
    uint16_t subdevice_id;
    uint8_t  revision_id;
};

struct amdgpu_drm_entry
{
    ULONG version;      /* AMDGPU_DRM_ENTRY_VERSION */
    ULONG size;         /* sizeof(struct amdgpu_drm_entry), for additions */

    int   (*open)(void);                                            /* drmOpen("amdgpu", "") */
    int   (*close)(int fd);                                         /* drmClose */
    int   (*ioctl)(int fd, unsigned long request, void *arg);       /* drmIoctl */
    int   (*command)(int fd, unsigned long index, void *data,
                     unsigned long size);                           /* drmCommandWriteRead */
    int   (*get_version)(int fd, int *major, int *minor, int *patchlevel,
                         char *name, int namelen);                  /* drmGetVersion */
    int   (*get_pci_info)(int fd, struct amdgpu_drm_pci_info *info); /* drmGetDevice2 */
    void *(*map)(int fd, uint32_t handle);                          /* drmMMap */
    void  (*unmap)(int fd, uint32_t handle);                        /* drmMUnmap */
    int   (*dup)(int fd);                                           /* drmDupFd: dup() of a DRM fd */
};

/* Inside amdgpu.hidd */
const struct amdgpu_drm_entry *HIDDAmdgpuDrmEntry(void);

/*
 * amdgpu.hidd's exported function, the first user LVO of the module:
 *
 *     const struct amdgpu_drm_entry *AmdgpuDrmEntry(void)
 *
 * A caller opens "amdgpu.hidd" and calls through the base it got.
 */
#define AmdgpuDrmEntry(base) \
    AROS_LC0(const struct amdgpu_drm_entry *, AmdgpuDrmEntry, \
             struct Library *, (base), 5, Amdgpu)

#endif /* LIBDRM_DRM_ENTRY_H */
