#ifndef _LAVAPIPE_INTERN_H
#define _LAVAPIPE_INTERN_H

/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <stdint.h>
#include <stdbool.h>

#include LC_LIBDEFS_FILE

#define CLID_Hidd_Vulkan_Lavapipe  "hidd.vulkan.lavapipe"

struct HiddVulkanLavapipeData
{
    OOP_Object *lavapipe_obj;
};

struct lavapipestaticdata
{
    OOP_Class       *vulkanclass;
    OOP_AttrBase    hiddVulkanAB;
    struct Library  *UtilityBase;
};

LIBBASETYPE
{
    struct Library              LibNode;
    struct lavapipestaticdata   sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#ifdef __cplusplus
extern "C" {
#endif
void Lavapipe_ForceLLVMPipeRTTI(void);
#ifdef __cplusplus
}
#endif

#endif
