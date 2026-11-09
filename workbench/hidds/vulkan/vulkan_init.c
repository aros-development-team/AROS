/*
    Copyright 2025-2026, The AROS Development Team. All rights reserved.

    Vulkan HIDD initialization - mirrors gallium_init.c.
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <hidd/vulkan_hidd.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "vulkan_intern.h"

static int HiddVulkan_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.vulkanAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Vulkan);

    return TRUE;
}

static int HiddVulkan_InitLib(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->sd.vulkanAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Vulkan);
    LIBBASE->sd.mbase = OOP_GetMethodID(IID_Hidd_Vulkan, 0);

    return (LIBBASE->sd.vulkanAttrBase != 0);
}

ADD2INITLIB(HiddVulkan_InitLib, 0)
ADD2EXPUNGELIB(HiddVulkan_ExpungeLib, 0)
