#ifndef VULKAN_INTERN_H
#define VULKAN_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vulkan.library: the Vulkan loader, dispatch and window system
    integration. The drivers are vulkan.hidd objects; the library takes each
    one's vkGetInstanceProcAddr and from there on speaks plain Vulkan to it.

    Dispatch: the first pointer of every dispatchable object a driver makes
    belongs to the loader (the ICD contract; Mesa's VK_LOADER_DATA). The
    library stores a struct vkl_dispatch pointer there:

      physical device       -> its struct vkl_icd_instance (first member)
      device, queue,
      command buffer        -> their struct vkl_device (first member)

    and the public entry points jump through it. A VkInstance is the
    library's own struct vkl_instance, holding one instance per driver.
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <stddef.h>
#include <stdint.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_aros.h>

#include "vulkan_dispatch.h"

struct VulkanLibBase
{
    struct Library  lib;
};

/* ---- the command table (vulkan_tramp.c) ---- */

#define VKL_LEVEL_GLOBAL    0
#define VKL_LEVEL_INSTANCE  1
#define VKL_LEVEL_PHYSDEV   2
#define VKL_LEVEL_DEVICE    3

struct vkl_command
{
    const char         *name;
    PFN_vkVoidFunction  entry;      /* the library's public entry point */
    ULONG               offset;     /* of its pointer in struct vkl_dispatch */
    UBYTE               level;
};

extern const struct vkl_command vkl_commands[VKL_NUM_COMMANDS];

const struct vkl_command *vkl_find_command(const char *name);

#define VKL_SLOT(disp, cmd) \
    (*(PFN_vkVoidFunction *)((UBYTE *)(disp) + (cmd)->offset))

/* ---- drivers ---- */

#define VKL_MAX_ICDS        4
#define VKL_MAX_PHYSDEVS    16

struct vkl_icd
{
    OOP_Object                 *obj;        /* the vulkan.hidd object */
    OOP_Object                 *display;    /* the display it came from, or NULL */
    PFN_vkGetInstanceProcAddr   gipa;
    BOOL                        software;
};

extern struct vkl_icd vkl_icds[VKL_MAX_ICDS];
extern ULONG vkl_icd_count;

BOOL vkl_icds_init(void);
void vkl_icds_exit(void);

/* ---- instance ---- */

struct vkl_instance;

struct vkl_icd_instance
{
    struct vkl_dispatch         disp;       /* first: physical devices point here */
    struct vkl_instance        *instance;
    struct vkl_icd             *icd;
    VkInstance                  handle;     /* the driver's instance */
    PFN_vkGetDeviceProcAddr     gdpa;
};

#define VKL_INSTANCE_MAGIC  0x564B4C49      /* 'VKLI' */

struct vkl_instance
{
    const struct vkl_dispatch  *loader;     /* first: &icd[0].disp */
    ULONG                       magic;
    ULONG                       icdCount;
    BOOL                        khrSurface; /* the WSI extensions enabled */
    BOOL                        arosSurface;
    struct SignalSemaphore      lock;
    struct MinList              surfaces;
    struct vkl_icd_instance     icd[VKL_MAX_ICDS];
};

#define VKL_DISPATCH(h)             (*(const struct vkl_dispatch * const *)(h))
#define VKL_INSTANCE(h)             ((struct vkl_instance *)(h))
#define VKL_INSTANCE_DISPATCH(h)    (&VKL_INSTANCE(h)->icd[0].disp)
#define VKL_ICD_INSTANCE(h)         (VKL_INSTANCE(h)->icd[0].handle)
#define VKL_PHYSDEV_ICD(h)          ((struct vkl_icd_instance *)VKL_DISPATCH(h))
#define VKL_SET_DISPATCH(h, d)      (*(const struct vkl_dispatch **)(h) = (d))

/* ---- device ---- */

#define VKL_MAX_QUEUES  16

struct vkl_device
{
    struct vkl_dispatch         disp;       /* first: device, queues, command buffers point here */
    struct vkl_icd_instance    *icd;
    VkPhysicalDevice            phys;
    VkDevice                    handle;
    BOOL                        swapchain;  /* VK_KHR_swapchain enabled */
    BOOL                        flipExts;   /* the driver imports display buffers */

    /* the driver's own entries for what the library overrides */
    struct vkl_dispatch         drv;

    /* the queue the library submits on by itself (image acquisition), and
       the lock that keeps the application's use of it apart */
    struct SignalSemaphore      qlock;
    VkQueue                     sigqueue;
    uint32_t                    sigfamily;

    /* queue -> family, as handed out */
    struct { VkQueue queue; uint32_t family; } queues[VKL_MAX_QUEUES];
    ULONG                       queueCount;
};

#define VKL_DEVICE(h)   ((struct vkl_device *)VKL_DISPATCH(h))

/* physical device entries the library provides (vulkan_device.c, vulkan_wsi.c) */
void vkl_device_install(struct vkl_icd_instance *ii);
void vkl_wsi_install_physdev(struct vkl_icd_instance *ii);
void vkl_wsi_install_device(struct vkl_device *dev);
void vkl_wsi_instance_cleanup(struct vkl_instance *inst);
PFN_vkVoidFunction vkl_wsi_instance_proc(struct vkl_instance *inst, const char *name);

BOOL vkl_ext_hidden_instance(const char *name);
BOOL vkl_ext_hidden_device(const char *name);

VkResult vkl_queue_submit(struct vkl_device *dev, VkQueue queue, uint32_t count,
                          const VkSubmitInfo *submits, VkFence fence);
uint32_t vkl_queue_family(struct vkl_device *dev, VkQueue queue);

/* ---- memory ---- */

APTR vkl_alloc(ULONG size);
void vkl_free(APTR mem);

#endif /* VULKAN_INTERN_H */
