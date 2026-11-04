/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _XF86DRM_H_
#define _XF86DRM_H_

/* libdrm's public interface, reduced to what the nouveau library uses;
   the calls are answered by the in-process shim in libdrm/arosdrm.c */
#include <libdrm/arosdrm.h>

#include <stdint.h>
#include <sys/types.h>

#define DRM_DIR_NAME            "/dev/dri"
#define DRM_DEV_NAME            "%s/card%d"
#define DRM_RENDER_MINOR_NAME   "renderD"

extern int drmCloseBufferHandle(int fd, uint32_t handle);
extern int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd);
extern int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle);

/* Device enumeration, reduced to the PCI case the nouveau library asks for */
#define DRM_BUS_PCI        0

typedef struct _drmPciBusInfo {
    uint16_t domain;
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
} drmPciBusInfo, *drmPciBusInfoPtr;

typedef struct _drmPciDeviceInfo {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t subvendor_id;
    uint16_t subdevice_id;
    uint8_t revision_id;
} drmPciDeviceInfo, *drmPciDeviceInfoPtr;

typedef struct _drmDevice {
    char **nodes;
    int available_nodes;
    int bustype;
    union {
        drmPciBusInfoPtr pci;
    } businfo;
    union {
        drmPciDeviceInfoPtr pci;
    } deviceinfo;
} drmDevice, *drmDevicePtr;

extern int  drmGetDevice2(int fd, uint32_t flags, drmDevicePtr *device);
extern void drmFreeDevice(drmDevicePtr *device);

#endif
