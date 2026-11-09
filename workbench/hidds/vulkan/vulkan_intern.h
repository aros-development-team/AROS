#ifndef _VULKAN_INTERN_H
#define _VULKAN_INTERN_H

/*
    Copyright 2025-2026, The AROS Development Team. All rights reserved.

    Internal header for the Vulkan base HIDD.
*/

#include <stdint.h>
#include <stdbool.h>

#include <exec/semaphores.h>
#include <proto/exec.h>

#include LC_LIBDEFS_FILE

struct HiddVulkanData
{
    APTR            gipa;       /* the driver's vkGetInstanceProcAddr, once asked for */
    APTR            instance;   /* the first VkInstance made: device commands resolve through it */
};

struct vulkanstaticdata
{
    OOP_Class       *vulkanclass;
    OOP_AttrBase    vulkanAttrBase;
    OOP_MethodID    mbase;      /* IID_Hidd_Vulkan method base */
};

LIBBASETYPE
{
    struct Library              LibNode;
    struct vulkanstaticdata     sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#endif /* _VULKAN_INTERN_H */
