/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The DRM device as a table.

    nouveau.hidd carries the kernel driver and its libdrm; the display and
    gallium code reach them by static linkage. Code with a libdrm of its
    own goes through this table instead: the Vulkan driver linked into
    the module (HIDDNVGfxDrmEntry()), and programs outside it, through
    the one function nvgfx.hidd exports, NVGfxDrmEntry().

    Every entry keeps the semantics of the libdrm function named beside
    it. An fd is what open() hands out and has meaning only through this
    table; it is not a file descriptor of any C library.
*/

#ifndef LIBDRM_DRM_ENTRY_H
#define LIBDRM_DRM_ENTRY_H

#include <exec/types.h>
#include <aros/libcall.h>
#include <stdint.h>

#define NOUVEAU_DRM_ENTRY_VERSION   1

struct nouveau_drm_pci_info
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

struct nouveau_drm_entry
{
    ULONG version;      /* NOUVEAU_DRM_ENTRY_VERSION */
    ULONG size;         /* sizeof(struct nouveau_drm_entry), for additions */

    int   (*open)(void);                                            /* drmOpen("nouveau", "") */
    int   (*close)(int fd);                                         /* drmClose */
    int   (*ioctl)(int fd, unsigned long request, void *arg);       /* drmIoctl */
    int   (*command)(int fd, unsigned long index, void *data,
                     unsigned long size);                           /* drmCommandWriteRead */
    int   (*get_version)(int fd, int *major, int *minor, int *patchlevel,
                         char *name, int namelen);                  /* drmGetVersion */
    int   (*get_pci_info)(int fd, struct nouveau_drm_pci_info *info); /* drmGetDevice2 */
    void *(*map)(int fd, uint32_t handle);                          /* drmMMap */
    void  (*unmap)(int fd, uint32_t handle);                        /* drmMUnmap */

    /* Added later: present when size covers it. A buffer of the display
     * (a scan-out bitmap's, by its GEM handle in the display's DRM client)
     * as a handle in client fd - the import a dma-buf would give. */
    int   (*import_display_handle)(int fd, uint32_t display_handle, uint32_t *handle);

    /* Added later: drmMMap with the notification libdrm_nouveau's
     * nouveau_bo_map relies on - unmapped(data) is called when the kernel
     * moves the object and the mapping goes stale. */
    void *(*map_notify)(int fd, uint32_t handle, void (*unmapped)(void *), void *data);
};

/* Inside nvgfx.hidd */
const struct nouveau_drm_entry *HIDDNVGfxDrmEntry(void);
/* The display's DRM client, which import_display_handle takes buffers from */
void HIDDNVGfxDrmEntrySetDisplay(int display_fd);

/*
 * nvgfx.hidd's exported function, the first user LVO of the module:
 *
 *     const struct nouveau_drm_entry *NVGfxDrmEntry(void)
 *
 * A caller opens "nvgfx.hidd" and calls through the base it got.
 */
#define NVGfxDrmEntry(base) \
    AROS_LC0(const struct nouveau_drm_entry *, NVGfxDrmEntry, \
             struct Library *, (base), 5, NVGfx)

#endif /* LIBDRM_DRM_ENTRY_H */
