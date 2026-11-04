/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Forced into every libnvk source: NVK's libdrm calls go to nvk_drm.c's
    implementation (over nouveau's DRM entry table, with its stack
    guard) under names of their own, so they never resolve to any
    other libdrm linked beside libnvk (nvk.nvdd links it on its own today).
    Generated from the drm* functions nvk_drm.c defines.
*/

#ifndef NVK_DRM_NAMES_H
#define NVK_DRM_NAMES_H

#define drmClose                     nvk_drmClose
#define drmCloseBufferHandle         nvk_drmCloseBufferHandle
#define drmCommandNone               nvk_drmCommandNone
#define drmCommandRead               nvk_drmCommandRead
#define drmCommandWrite              nvk_drmCommandWrite
#define drmCommandWriteRead          nvk_drmCommandWriteRead
#define drmFreeDevice                nvk_drmFreeDevice
#define drmFreeDevices               nvk_drmFreeDevices
#define drmFreeVersion               nvk_drmFreeVersion
#define drmGetCap                    nvk_drmGetCap
#define drmGetDevice2                nvk_drmGetDevice2
#define drmGetDeviceFromDevId        nvk_drmGetDeviceFromDevId
#define drmGetDeviceNameFromFd2      nvk_drmGetDeviceNameFromFd2
#define drmGetDevices2               nvk_drmGetDevices2
#define drmGetMagic                  nvk_drmGetMagic
#define drmGetNodeTypeFromFd         nvk_drmGetNodeTypeFromFd
#define drmGetRenderDeviceNameFromFd nvk_drmGetRenderDeviceNameFromFd
#define drmGetVersion                nvk_drmGetVersion
#define drmIoctl                     nvk_drmIoctl
#define drmMMap                      nvk_drmMMap
#define drmMUnmap                    nvk_drmMUnmap
#define drmOpen                      nvk_drmOpen
#define drmPrimeFDToHandle           nvk_drmPrimeFDToHandle
#define drmPrimeHandleToFD           nvk_drmPrimeHandleToFD
#define drmSyncobjCreate             nvk_drmSyncobjCreate
#define drmSyncobjDestroy            nvk_drmSyncobjDestroy
#define drmSyncobjEventfd            nvk_drmSyncobjEventfd
#define drmSyncobjExportSyncFile     nvk_drmSyncobjExportSyncFile
#define drmSyncobjFDToHandle         nvk_drmSyncobjFDToHandle
#define drmSyncobjHandleToFD         nvk_drmSyncobjHandleToFD
#define drmSyncobjImportSyncFile     nvk_drmSyncobjImportSyncFile
#define drmSyncobjQuery              nvk_drmSyncobjQuery
#define drmSyncobjQuery2             nvk_drmSyncobjQuery2
#define drmSyncobjReset              nvk_drmSyncobjReset
#define drmSyncobjSignal             nvk_drmSyncobjSignal
#define drmSyncobjTimelineSignal     nvk_drmSyncobjTimelineSignal
#define drmSyncobjTimelineWait       nvk_drmSyncobjTimelineWait
#define drmSyncobjTransfer           nvk_drmSyncobjTransfer
#define drmSyncobjWait               nvk_drmSyncobjWait

#endif /* NVK_DRM_NAMES_H */
